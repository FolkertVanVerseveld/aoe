#include "server.hpp"

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

void NetPkg::ntoh() {
	if (hdr.native_ordering)
		return;

	switch ((NetPkgType)ntohs(hdr.type)) {
		case NetPkgType::set_protocol:
		case NetPkgType::chat_text: {
			uint16_t *dw = (uint16_t*)data.data();
			dw[0] = ntohs(dw[0]);
			break;
		}
		case NetPkgType::set_scn_vars: {
			uint32_t *dd = (uint32_t*)data.data();

			for (unsigned i = 0; i < 10; ++i)
				dd[i] = ntohl(dd[i]);

			break;
		}
		case NetPkgType::start_game:
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
		case NetPkgType::set_protocol:
		case NetPkgType::chat_text: {
			uint16_t *dw = (uint16_t*)data.data();
			dw[0] = htons(dw[0]);
			break;
		}
		case NetPkgType::set_scn_vars: {
			uint32_t *dd = (uint32_t*)data.data();

			for (unsigned i = 0; i < 10; ++i)
				dd[i] = htonl(dd[i]);

			break;
		}
		case NetPkgType::start_game:
			// no payload
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

	// read data
	this->data.resize(need);

	if (q.size() < need + NetPkgHdr::size)
		throw std::runtime_error("missing pkg data");

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
	hdr.payload = (uint16_t)data.size();
}

void NetPkg::set_protocol(uint16_t version) {
	data.resize(2);

	uint16_t *dw = (uint16_t*)data.data();
	*dw = version;

	set_hdr(NetPkgType::set_protocol);
}

uint16_t NetPkg::protocol_version() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::set_protocol || data.size() != 2)
		throw std::runtime_error("not a protocol network packet");

	const uint16_t *dw = (const uint16_t *)data.data();
	return *dw;
}

void NetPkg::set_chat_text(const std::string &s) {
	assert(s.size() <= max_payload - 2);

	size_t n = s.size();
	data.resize(2u + n);

	uint16_t *dw = (uint16_t *)data.data();

	dw[0] = (uint16_t)n;
	memcpy(&dw[1], s.data(), n);

	set_hdr(NetPkgType::chat_text);
}

std::string NetPkg::chat_text() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::chat_text || data.size() > max_payload - 2)
		throw std::runtime_error("not a chat text packet");

	const uint16_t *dw = (const uint16_t *)data.data();

	uint16_t n = dw[0];

	std::string s(n, ' ');
	memcpy(s.data(), &dw[1], n);

	return s;
}

void NetPkg::set_start_game() {
	data.clear();
	set_hdr(NetPkgType::start_game);
}

void NetPkg::set_scn_vars(const ScenarioSettings &scn) {
	data.clear();

	data.resize(10 * sizeof(uint32_t));

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = scn.width;
	dd[1] = scn.height;
	dd[2] = scn.popcap;
	dd[3] = scn.age;
	dd[4] = scn.seed;
	dd[5] = scn.villagers;

	dd[6] = scn.res.food;
	dd[7] = scn.res.wood;
	dd[8] = scn.res.gold;
	dd[9] = scn.res.stone;

	uint8_t flags = 0;

	if (scn.fixed_start     ) flags |= 1 << 0;
	if (scn.explored        ) flags |= 1 << 1;
	if (scn.all_technologies) flags |= 1 << 2;
	if (scn.cheating        ) flags |= 1 << 3;
	if (scn.square          ) flags |= 1 << 4;
	if (scn.restricted      ) flags |= 1 << 5;

	data.emplace_back(flags);

	set_hdr(NetPkgType::set_scn_vars);
}

ScenarioSettings NetPkg::get_scn_vars() {
	ntoh();

	size_t size = 10 * sizeof(uint32_t) + 1;

	if ((NetPkgType)hdr.type != NetPkgType::set_scn_vars || data.size() != size)
		throw std::runtime_error("not a scenario settings variables packet");

	const uint32_t *dd = (const uint32_t*)data.data();

	ScenarioSettings scn;

	scn.width     = dd[0];
	scn.height    = dd[1];
	scn.popcap    = dd[2];
	scn.age       = dd[3];
	scn.seed      = dd[4];
	scn.villagers = dd[5];

	scn.res.food  = dd[6];
	scn.res.wood  = dd[7];
	scn.res.gold  = dd[8];
	scn.res.stone = dd[9];

	const uint8_t *db = (const uint8_t*)&dd[10];

	uint8_t flags = db[0];

	scn.fixed_start      = !!(flags & (1 << 0));
	scn.explored         = !!(flags & (1 << 1));
	scn.all_technologies = !!(flags & (1 << 2));
	scn.cheating         = !!(flags & (1 << 3));
	scn.square           = !!(flags & (1 << 4));
	scn.restricted       = !!(flags & (1 << 5));

	return scn;
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
