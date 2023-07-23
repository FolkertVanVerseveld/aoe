#pragma once

#include <vector>
#include <cstdint>

#include "entity_info.hpp"

namespace aoe {

enum class TileType {
	unexplored,
	desert,
	grass,
	water,
	deepwater,
	grass_desert,
	water_desert,
	deepwater_water,
};

enum class TerrainType {
	islands,
	continents,
	normal,
	flat,
	bumpy,
	max,
};

// TODO introduce terrain block/chunk
typedef uint16_t tile_t;

class Terrain final {
	std::vector<tile_t> tiles;
	std::vector<uint8_t> hmap;
	std::vector<bool> obstructed; // tiles occupied by buildings
public:
	unsigned w, h, seed, players;
	bool wrap;

	Terrain();

	void resize(unsigned width, unsigned height, unsigned seed, unsigned players, bool wrap);

	void generate();

	static constexpr tile_t tile_id(TileType type, unsigned subimage) noexcept {
		return ((unsigned)type & 0x7) | (subimage << 3);
	}

	static constexpr TileType tile_type(tile_t id) noexcept {
		return (TileType)(id & 0x7);
	}

	static constexpr unsigned tile_img(tile_t id) noexcept {
		return id >> 3;
	}

	static constexpr bool tile_hasoverlay(tile_t v) noexcept {
		TileType t = tile_type(v);
		return t == TileType::deepwater_water || t == TileType::grass_desert;
	}

	static constexpr TileType tile_base(tile_t v) noexcept {
		TileType type = tile_type(v);
		if (type == TileType::deepwater_water)
			return TileType::deepwater;
		else if (type == TileType::grass_desert)
			return TileType::grass;

		return type;
	}

	tile_t tile_at(unsigned x, unsigned y);
	uint8_t h_at(unsigned x, unsigned y);

	void add_building(EntityType t, unsigned x, unsigned y);

	void fetch(std::vector<tile_t> &tiles, std::vector<uint8_t> &hmap, unsigned x, unsigned y, unsigned &w, unsigned &h);

	void set(const std::vector<tile_t> &tiles, const std::vector<uint8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);
};

}
