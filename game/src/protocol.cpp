#include "server.hpp"

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

	switch ((NetPkgType)hdr.type) {
		case NetPkgType::set_protocol: {
			uint16_t *dw = (uint16_t*)data.data();
			dw[0] = ntohs(dw[0]);
			break;
		}
		default:
			throw std::runtime_error("bad type");
	}

	hdr.ntoh();
}

void NetPkg::hton() {
	if (!hdr.native_ordering)
		return;

	switch ((NetPkgType)hdr.type) {
		case NetPkgType::set_protocol: {
			uint16_t *dw = (uint16_t*)data.data();
			dw[0] = htons(dw[0]);
			break;
		}
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

	for (unsigned i = 0; i < need; ++i)
		this->data[i] = q[i + NetPkgHdr::size];

	// convert to native ordering
	ntoh();

	// all good, remove from q
	for (unsigned i = 0; i < need + NetPkgHdr::size; ++i)
		q.pop_front();
}

void NetPkg::set_protocol(uint16_t version) {
	hdr.type = (unsigned)NetPkgType::set_protocol;
	hdr.native_ordering = true;
	data.resize(2);

	uint16_t *dw = (uint16_t*)data.data();
	*dw = version;

	hdr.payload = data.size();
}

uint16_t NetPkg::protocol_version() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::set_protocol || hdr.payload != 2)
		throw std::runtime_error("not a protocol network packet");

	const uint16_t *dw = (const uint16_t *)data.data();
	return *dw;
}

NetPkgType NetPkg::type() {
	ntoh();
	return (NetPkgType)hdr.type;
}

void Client::send(NetPkg &pkg) {
	pkg.hton();
	send(&pkg.hdr.type);
	send(&pkg.hdr.payload);
	send(pkg.data.data(), pkg.data.size());
}

NetPkg Client::recv() {
	NetPkg pkg;
	unsigned payload;

	recv(&pkg.hdr.type);
	recv(&pkg.hdr.payload);

	payload = ntohs(pkg.hdr.payload);
	pkg.data.resize(payload);
	recv(pkg.data.data(), payload);

	pkg.ntoh();

	return pkg;
}

}
