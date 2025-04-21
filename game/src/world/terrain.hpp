#pragma once

#include <vector>
#include <cstdint>
#include <array>

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
	islands,    // small islands
	continents, // large islands
	normal,     // coastal
	flat,       // inland
	bumpy,      // highland
	max,
};

// TODO introduce terrain block/chunk
typedef uint16_t tile_t;

static constexpr bool is_water(TileType t) {
	// NOTE TileType::water_desert doesn't count!
	return t == TileType::water || t == TileType::deepwater || t == TileType::deepwater_water;
}

extern std::array<unsigned, 8> heights(const std::vector<uint8_t> &hmap, size_t w, size_t h, size_t x, size_t y, unsigned f);
extern std::array<unsigned, 4> hhvn(const std::vector<uint8_t> &hmap, size_t w, size_t h, size_t x, size_t y, unsigned hdef);
extern std::array<unsigned, 4> hdn(const std::vector<uint8_t> &hmap, size_t w, size_t h, size_t x, size_t y, unsigned hdef);

extern std::array<TileType, 8> neighs(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y, TileType f);
extern std::array<TileType, 4> fhvn(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y, TileType f);
extern std::array<TileType, 4> neighs_hv(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y);
extern std::array<TileType, 4> fdn(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y, TileType f);

class Terrain final {
	std::vector<tile_t> tiles;
	std::vector<uint8_t> hmap;
	std::vector<bool> obstructed; // tiles occupied by buildings
public:
	unsigned w, h, seed, players;
	bool wrap;
	TerrainType type;

	// TODO increase max_size on demand
	static constexpr unsigned min_size = 48, chunk_size = 16, max_size = 250;

	Terrain();

	void resize(unsigned width, unsigned height, unsigned seed, unsigned players, bool wrap, TerrainType type);

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
private:
	void tgen_desert();
	void tgen_normal();
	void tgen_flat();

	void fix_tile_transitions();
	void fix_water_desert();
	void fix_grass_desert();
	void smooth_water();

	void fix_heightmap();
	void fix_water_transitions();
	void smooth_slopes();
};

}
