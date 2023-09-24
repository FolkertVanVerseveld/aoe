#include "../terrain.hpp"

#include <cassert>

#include <array>

namespace aoe {

enum class TerrainAlgorithm {
	desert_garbage,
	perlin_noise,
};

static const TerrainAlgorithm alg = TerrainAlgorithm::perlin_noise;

const std::array<std::pair<int, int>, 8> neighs_dir{{
	{-1,  1}, {0,  1}, {1,  1},
	{-1,  0},          {1,  0},
	{-1, -1}, {0, -1}, {1, -1},
}};

void Terrain::tgen_desert() {
	// TODO use real generator like perlin noise
	TileType types[] = { TileType::desert, TileType::grass, TileType::grass_desert };

	for (size_t i = 0, n = tiles.size(); i < n; ++i) {
		tiles[i] = Terrain::tile_id(TileType::desert, rand() % 9);
		hmap[i] = 0;
	}
}

static constexpr inline size_t pos_dir(size_t w, size_t x, size_t y, unsigned dir) {
	auto d = neighs_dir[dir];
	return (size_t)((long)y + d.second) * w + (size_t)((long)x + d.first);
}

void Terrain::generate() {
	switch (alg) {
	case TerrainAlgorithm::perlin_noise:
		switch (type) {
		case TerrainType::normal:
			tgen_normal();
			break;
		case TerrainType::flat:
			tgen_flat();
			break;
		default:
			fprintf(stderr, "%s: stub terrain type=%u, falling back to desert garbage\n", __func__, (unsigned)type);
			tgen_desert();
			break;
		}
		break;
	case TerrainAlgorithm::desert_garbage:
	default:
		tgen_desert();
		break;
	}

	// fix tile transitions
	// fix water desert transition
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			size_t idx = y * w + x;
			// prefer a slow and dumb algorithm that just works
			TileType type = tile_type(tiles[idx]);

			if (!is_water(type))
				continue;

			// force water on lowest elevation
			hmap[idx] = 0;

			auto nn = neighs(tiles, w, h, x, y, TileType::water);

			for (unsigned i = 0; i < nn.size(); ++i) {
				TileType tt = nn[i];
				if (!is_water(tt)) {
					size_t idx2 = pos_dir(w, x, y, i);
					tiles[idx2] = tile_id(TileType::water_desert, 0);
				}
			}
		}
	}

	// second pass water transitions
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			size_t idx = y * w + x;
			TileType type = tile_type(tiles[idx]);
			if (type == TileType::water_desert) {
				auto nn = fhvn(tiles, w, h, x, y, TileType::desert);
				unsigned edges = 0, subimage = 0;

				for (TileType t : nn)
					if (t == TileType::water)
						++edges;

				switch (edges) {
				case 1:
					if (nn[0] == TileType::water)
						subimage = 11;
					else if (nn[1] == TileType::water)
						subimage = 8;
					else if (nn[2] == TileType::water)
						subimage = 9;
					else
						subimage = 10;
					break;
				case 2:
					if (nn[1] == TileType::water && nn[3] == TileType::water)
						subimage = 0;
					else if (nn[1] == TileType::water && nn[0] == TileType::water)
						subimage = 1;
					else if (nn[2] == TileType::water && nn[3] == TileType::water)
						subimage = 2;
					else
						subimage = 3;
					break;
				case 0:
					// find diagonal neighbors
					nn = fdn(tiles, w, h, x, y, TileType::desert);

					if (nn[0] == TileType::water)
						subimage = 5;
					else if (nn[1] == TileType::water)
						subimage = 7;
					else if (nn[2] == TileType::water)
						subimage = 4;
					else
						subimage = 6;

					break;
				}

				tiles[idx] = Terrain::tile_id(type, subimage);
				hmap[idx] = 0;
			}
		}
	}

	// fix grass desert transitions
	// TODO add water deep water transitions
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			size_t idx = y * w + x;
			TileType type = tile_type(tiles[idx]);

			if (type == TileType::grass) {
				auto nn = fhvn(tiles, w, h, x, y, TileType::grass);
				unsigned bits = 0;

				if (nn[1] == TileType::desert || nn[1] == TileType::water_desert)
					bits |= 1 << 0;
				if (nn[0] == TileType::desert || nn[0] == TileType::water_desert)
					bits |= 1 << 1;
				if (nn[3] == TileType::desert || nn[3] == TileType::water_desert)
					bits |= 1 << 2;
				if (nn[2] == TileType::desert || nn[2] == TileType::water_desert)
					bits |= 1 << 3;

				if (bits) {
					type = TileType::grass_desert;
					bits <<= 16 - 4 - 3;
				}

				tiles[idx] = tile_id(type, bits);
			}
		}
	}

	// smooth heightmap. three steps
	// step one: force tiles adjacent to water_desert to have same hmap
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			size_t idx = y * w + x;
			TileType type = tile_type(tiles[idx]);

			if (type != TileType::water_desert)
				continue;

			unsigned h0 = hmap[idx];
			assert(h0 == 0);
			auto hh = heights(hmap, w, h, x, y, 1);

			for (unsigned i = 0; i < hh.size(); ++i) {
				if (hh[i] != 1) {
					size_t idx2 = pos_dir(w, x, y, i);
					hmap[idx2] = h0;
				}
			}
		}
	}

	// step two: smooth slopes. limit runs just in case to prevent looping forever
	for (size_t runs = 0; runs < std::max(w, h); ++runs) {
		for (size_t y = 0; y < h; ++y) {
			for (size_t x = 0; x < w; ++x) {
				size_t idx = y * w + x;
				unsigned h0 = hmap[idx];
				auto hh = heights(hmap, w, h, x, y, h0);

				for (unsigned i = 0; i < hh.size(); ++i) {
					if (hh[i] > h0 + 1) {
						size_t idx2 = pos_dir(w, x, y, i);
						hmap[idx2] = h0 + 1;
					}
				}
			}
		}
	}

	// TODO step three: convert flat tiles to hill tiles with corners etc.
}

}
