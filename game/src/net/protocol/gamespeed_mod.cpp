#include "../../server.hpp"

#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

NetGamespeedControl NetPkg::get_gamespeed() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::gamespeed_control)
		throw std::runtime_error("not a gamespeed control packet");

	args.clear();
	read("2B", args);
	NetGamespeedType type = (NetGamespeedType)u8(0);
	uint8_t speed = (uint8_t)u8(1);

	if (type > NetGamespeedType::decrease)
		throw std::runtime_error("invalid gamespeed control type");

	return NetGamespeedControl(type, speed);
}

void NetPkg::set_gamespeed(NetGamespeedType type, uint8_t speed) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::gamespeed_control);
	write("2B", pkgargs{ (uint8_t)type, speed}, false);
}

}
