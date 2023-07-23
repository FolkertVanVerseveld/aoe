#include "../../server.hpp"

#include <cassert>
#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::set_terrain_mod(const NetTerrainMod &tm) {
	ZoneScoped;
	assert(tm.tiles.size() == tm.hmap.size());
	size_t tsize = tm.w * tm.h * 2;

	if (tsize > max_payload - NetTerrainMod::possize)
		throw std::runtime_error("terrain mod too big");

	PkgWriter out(*this, NetPkgType::terrainmod);
	unsigned pos = write("4H", pkgargs{ tm.x, tm.y, tm.w, tm.h }, false);

	// TODO implement write(int ch, vector)
	for (uint16_t v : tm.tiles)
		pos += write("H", pkgargs{ v });

	for (uint8_t h : tm.hmap)
		pos += write("B", pkgargs{ h });
}

NetTerrainMod NetPkg::get_terrain_mod() {
	ZoneScoped;
	unsigned pos = read(NetPkgType::terrainmod, "4H");
	NetTerrainMod tm;

	tm.x = u16(0); tm.y = u16(1);
	tm.w = u16(2); tm.h = u16(3);

	size_t size = tm.w * tm.h;

	tm.tiles.clear();
	tm.hmap.clear();

	for (size_t i = 0; i < size; ++i) {
		args.clear();
		pos += read("H", args, pos);
		tm.tiles.emplace_back(u16(0));
	}

	for (size_t i = 0; i < size; ++i) {
		args.clear();
		pos += read("B", args, pos);
		tm.hmap.emplace_back(u8(0));
	}

	return tm;
}

}
