#include "../../server.hpp"

#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

NetGamespeedControl NetPkg::get_gamespeed() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::gamespeed_control)
		throw std::runtime_error("not a gamespeed control packet");

	args.clear();
	read("B", args);
	NetGamespeedType type = (NetGamespeedType)u8(0);

	if (type > NetGamespeedType::decrease)
		throw std::runtime_error("invalid gamespeed control type");

	return NetGamespeedControl(type);
}

void NetPkg::set_gamespeed(NetGamespeedType type) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::gamespeed_control);
	write("B", pkgargs{ (uint8_t)type }, false);
}

}
