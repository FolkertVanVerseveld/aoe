#include "../../server.hpp"

#include <cassert>
#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::set_resources(const Resources &res) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::resmod);

	clear();
	writef("4i", res.wood, res.food, res.gold, res.stone);
}

Resources NetPkg::get_resources() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::resmod)
		throw std::runtime_error("not a resources control packet");

	args.clear();
	read("4i", args);
	Resources res;

	res.wood  = i32(0);
	res.food  = i32(1);
	res.gold  = i32(2);
	res.stone = i32(3);

	return res;
}

}
