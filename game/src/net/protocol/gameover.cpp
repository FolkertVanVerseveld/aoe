#include "../../server.hpp"

#include <except.hpp>

namespace aoe {

void NetPkg::set_gameover(unsigned team) {
	PkgWriter out(*this, NetPkgType::gameover);
	write("H", pkgargs{ team }, false);
}

unsigned NetPkg::get_gameover() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::gameover)
		throw std::runtime_error("not a gameover packet");

	args.clear();
	read("H", args);
	return u16(0);
}

}
