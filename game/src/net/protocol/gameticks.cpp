#include "../../server.hpp"

#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

uint16_t NetPkg::get_gameticks() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::gameticks)
		throw std::runtime_error("not a gameticks packet");

	args.clear();
	read("H", args);

	return u16(0);
}

void NetPkg::set_gameticks(unsigned n) {
	ZoneScoped;
	assert(n <= UINT16_MAX);

	PkgWriter out(*this, NetPkgType::gameticks);
	clear();
	writef("H", n);
}

}
