#include "../server.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

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

	hdr.ntoh();
}

void NetPkg::hton() {
	if (!hdr.native_ordering)
		return;

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

void NetPkg::set_start_game(NetStartGameType type) {
	PkgWriter out(*this, NetPkgType::start_game);
	clear();
	writef("B", type);
}

NetStartGameType NetPkg::get_start_type() {
	read(NetPkgType::start_game, "B");
	return (NetStartGameType)u8(0);
}

NetPkgType NetPkg::type() {
	ntoh();
	return (NetPkgType)hdr.type;
}

void Client::send(NetPkg &pkg) {
	ZoneScoped;
	pkg.hton();

	std::lock_guard<std::mutex> lk(m);

	size_t payload = pkg.data.size(), n = 4 + payload;
	sendbuf.resize(n);

	// prepare header
	uint16_t *v = (uint16_t*)sendbuf.data();
	v[0] = pkg.hdr.type;
	v[1] = pkg.hdr.payload;

	memcpy(&v[2], pkg.data.data(), payload);
	send(sendbuf.data(), n);
}

NetPkg Client::recv() {
	ZoneScoped;
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
