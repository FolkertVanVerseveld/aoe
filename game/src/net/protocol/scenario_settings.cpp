#include "../../server.hpp"

#include <except.hpp>

namespace aoe {

void NetPkg::set_scn_vars(const ScenarioSettings &scn) {
	PkgWriter out(*this, NetPkgType::set_scn_vars);

	uint8_t flags = 0;

	if (scn.fixed_start     ) flags |= 1 << 0;
	if (scn.explored        ) flags |= 1 << 1;
	if (scn.all_technologies) flags |= 1 << 2;
	if (scn.cheating        ) flags |= 1 << 3;
	if (scn.square          ) flags |= 1 << 4;
	if (scn.wrap            ) flags |= 1 << 5;
	if (scn.restricted      ) flags |= 1 << 6;

	write("6I4IB", pkgargs({
		(uint64_t)scn.width, (uint64_t)scn.height, (uint64_t)scn.popcap, (uint64_t)scn.age, (uint64_t)scn.seed, (uint64_t)scn.villagers,
		(uint64_t)scn.res.food, (uint64_t)scn.res.wood, (uint64_t)scn.res.gold, (uint64_t)scn.res.stone,
		(uint64_t)flags,
	}), false);
}

ScenarioSettings NetPkg::get_scn_vars() {
	read(NetPkgType::set_scn_vars, "6I4IB");

	ScenarioSettings scn;
	uint8_t flags;

	scn.width = u32(0); scn.height = u32(1);
	scn.popcap = u32(2); scn.age = u32(3); scn.seed = u32(4); scn.villagers = u32(5);
	scn.res.food = u32(6); scn.res.wood = u32(7); scn.res.gold = u32(8); scn.res.stone = u32(9);

	flags = u8(10);

	scn.fixed_start      = !!(flags & (1 << 0));
	scn.explored         = !!(flags & (1 << 1));
	scn.all_technologies = !!(flags & (1 << 2));
	scn.cheating         = !!(flags & (1 << 3));
	scn.square           = !!(flags & (1 << 4));
	scn.wrap             = !!(flags & (1 << 5));
	scn.restricted       = !!(flags & (1 << 6));

	return scn;
}

}
