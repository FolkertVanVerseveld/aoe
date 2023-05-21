#include "../server.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

namespace aoe {

void NetPkgHdr::ntoh() {
	if (native_ordering)
		return;

	type = ntohs(type);
	payload = ntohs(payload);

	native_ordering = true;
}

void NetPkgHdr::hton() {
	if (!native_ordering)
		return;

	type = htons(type);
	payload = htons(payload);

	native_ordering = false;
}

void NetPkg::need_payload(size_t n) {
	if (data.size() < n)
		throw std::runtime_error("corrupt data");
}

void NetPkg::ntoh() {
	if (hdr.native_ordering)
		return;

	switch ((NetPkgType)ntohs(hdr.type)) {
		case NetPkgType::set_username:
		case NetPkgType::chat_text:
		case NetPkgType::set_protocol:
		case NetPkgType::playermod:
		case NetPkgType::gameover:
		case NetPkgType::particle_mod:
		case NetPkgType::entity_mod:
		case NetPkgType::resmod:
		case NetPkgType::gameticks:
		case NetPkgType::cam_set:
		case NetPkgType::set_scn_vars:
		case NetPkgType::terrainmod:
		case NetPkgType::peermod:
			// bytes are converted implicitly
			break;
		case NetPkgType::start_game:
		case NetPkgType::gamespeed_control:
			// no payload
			break;
		default:
			throw std::runtime_error("bad type");
	}

	hdr.ntoh();
}

void NetPkg::hton() {
	if (!hdr.native_ordering)
		return;

	switch ((NetPkgType)hdr.type) {
		case NetPkgType::set_username:
		case NetPkgType::set_protocol:
		case NetPkgType::chat_text:
		case NetPkgType::playermod:
		case NetPkgType::gameover:
		case NetPkgType::particle_mod:
		case NetPkgType::entity_mod:
		case NetPkgType::resmod:
		case NetPkgType::cam_set:
		case NetPkgType::gameticks:
		case NetPkgType::set_scn_vars:
		case NetPkgType::terrainmod:
		case NetPkgType::peermod:
			// bytes are converted implicitly
			break;
		case NetPkgType::start_game:
			// no payload
			break;
		case NetPkgType::gamespeed_control:
			// 1 byte, don't do anything
			break;
		default:
			throw std::runtime_error("bad type");
	}

	hdr.hton();
}

void NetPkg::write(std::deque<uint8_t> &q) {
	hton();

	static_assert(NetPkgHdr::size == 4);

	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	data.v[0] = this->hdr.type;
	data.v[1] = this->hdr.payload;

	for (unsigned i = 0; i < NetPkgHdr::size; ++i)
		q.emplace_back(data.b[i]);

	for (unsigned i = 0; i < this->data.size(); ++i)
		q.emplace_back(this->data[i]);
}

void NetPkg::write(std::vector<uint8_t> &q) {
	hton();

	static_assert(NetPkgHdr::size == 4);

	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	data.v[0] = this->hdr.type;
	data.v[1] = this->hdr.payload;

	for (unsigned i = 0; i < NetPkgHdr::size; ++i)
		q.emplace_back(data.b[i]);

	for (unsigned i = 0; i < this->data.size(); ++i)
		q.emplace_back(this->data[i]);
}

NetPkg::NetPkg(std::deque<uint8_t> &q) : hdr(0, 0, false), data() {
	if (q.size() < NetPkgHdr::size)
		throw std::runtime_error("bad pkg hdr");

	// read pkg hdr
	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	static_assert(sizeof(data) == NetPkgHdr::size);

	data.b[0] = q[0];
	data.b[1] = q[1];
	data.b[2] = q[2];
	data.b[3] = q[3];

	this->hdr = NetPkgHdr(data.v[0], data.v[1], false);
	unsigned need = ntohs(this->hdr.payload);

	if (q.size() < need + NetPkgHdr::size)
		throw std::runtime_error("missing pkg data");

	// read data
	this->data.resize(need);

	for (unsigned i = 0; i < need; ++i)
		this->data[i] = q[i + NetPkgHdr::size];

	// convert to native ordering
	ntoh();

	// all good, remove from q
	for (unsigned i = 0; i < need + NetPkgHdr::size; ++i)
		q.pop_front();
}

void NetPkg::set_hdr(NetPkgType type) {
	assert(data.size() <= max_payload);

	hdr.native_ordering = true;
	hdr.type = (unsigned)type;

	if (data.size() > NetPkg::max_payload)
		throw std::runtime_error("payload overflow");

	hdr.payload = (uint16_t)data.size();
}

void NetPkg::set_start_game() {
	data.clear();
	set_hdr(NetPkgType::start_game);
}

NetPeerControl NetPkg::get_peer_control() {
	ZoneScoped;
	unsigned pos = read(NetPkgType::peermod, "2IH");

	const uint32_t *dd = (const uint32_t*)data.data();
	const uint16_t *dw = (const uint16_t*)&dd[2];

	IdPoolRef ref{ u32(0), u32(1) };
	NetPeerControlType type = (NetPeerControlType)u16(2);

	switch (type) {
		case NetPeerControlType::set_username:
			pos += read("40s", args, pos);
			return NetPeerControl(ref, str(3));
		case NetPeerControlType::set_player_idx:
			pos += read("H", args, pos);
			return NetPeerControl(ref, type, u16(3));
		default:
			return NetPeerControl(ref, type);
	}
}

NetPkgType NetPkg::type() {
	ntoh();
	return (NetPkgType)hdr.type;
}

void Client::send(NetPkg &pkg) {
	pkg.hton();

	// prepare header
	uint16_t v[2];
	v[0] = pkg.hdr.type;
	v[1] = pkg.hdr.payload;

	send(v, 2);
	send(pkg.data.data(), (int)pkg.data.size());
}

NetPkg Client::recv() {
	NetPkg pkg;
	unsigned payload;

	// retrieve header
	uint16_t dw[2];
	recv(dw, 2);

	// parse header
	pkg.hdr.type = dw[0];
	pkg.hdr.payload = dw[1];

	// reserve payload
	payload = ntohs(pkg.hdr.payload);
	pkg.data.resize(payload);

	// retrieve payload
	recv(pkg.data.data(), payload);

	pkg.ntoh();

	return pkg;
}

}
