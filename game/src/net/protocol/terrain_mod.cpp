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

	data.resize(NetTerrainMod::possize + tsize);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = tm.x; dw[1] = tm.y;
	dw[2] = tm.w; dw[3] = tm.h;

	uint8_t *db = (uint8_t*)&dw[4];

	memcpy(db, tm.tiles.data(), tm.tiles.size());
	memcpy(db + tm.tiles.size(), tm.hmap.data(), tm.hmap.size());

	set_hdr(NetPkgType::terrainmod);
}

NetTerrainMod NetPkg::get_terrain_mod() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::terrainmod || data.size() < NetTerrainMod::possize)
		throw std::runtime_error("not a terrain control packet");

	const uint16_t *dw = (const uint16_t*)data.data();

	NetTerrainMod tm;

	tm.x = dw[0]; tm.y = dw[1];
	tm.w = dw[2]; tm.h = dw[3];

	const uint8_t *db = (const uint8_t*)&dw[4];

	// TODO check packet size
	size_t size = tm.w * tm.h;

	tm.tiles.resize(size);
	tm.hmap.resize(size);

	memcpy(tm.tiles.data(), db, size);
	memcpy(tm.hmap.data(), db + size, size);

	return tm;
}

}
