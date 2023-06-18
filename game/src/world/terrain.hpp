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
};

enum class TerrainTile {
	unknown, // either empty or unexplored
	desert,
	grass,
	water,
	deepwater,
	water_desert,
	grass_desert,
	desert_overlay,
	deepwater_overlay,
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

class Terrain final {
	std::vector<uint8_t> tiles;
	std::vector<int8_t> hmap;
	std::vector<bool> obstructed; // tiles occupied by buildings
public:
	unsigned w, h, seed, players;
	bool wrap;

	Terrain();

	void resize(unsigned width, unsigned height, unsigned seed, unsigned players, bool wrap);

	void generate();

	static constexpr uint8_t tile_id(TileType type, unsigned subimage) noexcept {
		return ((unsigned)type & 0x7) | (subimage << 3);
	}

	static constexpr TileType tile_type(uint8_t id) noexcept {
		return (TileType)(id & 0x7);
	}

	static constexpr unsigned tile_img(uint8_t id) noexcept {
		return id >> 3;
	}

	uint8_t tile_at(unsigned x, unsigned y);
	int8_t h_at(unsigned x, unsigned y);

	void add_building(EntityType t, unsigned x, unsigned y);

	void fetch(std::vector<uint8_t> &tiles, std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned &w, unsigned &h);

	void set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);
};

}
