#include "server.hpp"

#include <stdexcept>
#include <string>

namespace aoe {

enum class NetPkgType {
	set_protocol,
};

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
	if (!hdr.native_ordering)
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
	if (hdr.native_ordering)
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

void NetPkg::set_protocol(uint16_t version) {
	hdr.type = (unsigned)NetPkgType::set_protocol;
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

void Client::send(NetPkg &pkg) {
	pkg.hton();
	send(&pkg.hdr.type, sizeof(pkg.hdr.type));
	send(&pkg.hdr.payload, sizeof(pkg.hdr.type));
	send(pkg.data.data(), pkg.data.size());
}

NetPkg Client::recv() {
	NetPkg pkg(0, 0);
	unsigned payload;

	recv(&pkg.hdr.type, sizeof(pkg.hdr.type));
	recv(&pkg.hdr.payload, sizeof(pkg.hdr.payload));

	payload = ntohs(pkg.hdr.payload);
	pkg.data.resize(payload);
	recv(pkg.data.data(), payload);

	pkg.ntoh();

	return pkg;
}

}
