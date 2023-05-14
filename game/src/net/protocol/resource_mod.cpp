#include "../../server.hpp"

#include <cassert>
#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::set_resources(const Resources &res) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::resmod);

	write("4I", std::initializer_list<netarg>{
		res.food, res.wood, res.gold, res.stone
	}, false);
}

Resources NetPkg::get_resources() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::resmod)
		throw std::runtime_error("not a resources control packet");

	args.clear();
	read("4I", args);
	Resources res;

	res.food  = u32(0);
	res.wood  = u32(1);
	res.gold  = u32(2);
	res.stone = u32(3);

	return res;
}

}
