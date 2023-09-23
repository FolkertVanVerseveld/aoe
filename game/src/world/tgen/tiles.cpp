#include "../terrain.hpp"

namespace aoe {

enum class TerrainAlgorithm {
	desert_garbage,
	perlin_noise,
};

static const TerrainAlgorithm alg = TerrainAlgorithm::perlin_noise;

void Terrain::tgen_desert() {
	// TODO use real generator like perlin noise
	TileType types[] = { TileType::desert, TileType::grass, TileType::grass_desert };

	for (size_t i = 0, n = tiles.size(); i < n; ++i) {
		tiles[i] = Terrain::tile_id(TileType::desert, rand() % 9);
		hmap[i] = 0;
	}
}

void Terrain::generate() {
	switch (alg) {
	case TerrainAlgorithm::perlin_noise:
		switch (type) {
		case TerrainType::normal:
			tgen_normal();
			return;
		case TerrainType::flat:
			tgen_flat();
			return;
		default:
			fprintf(stderr, "%s: stub terrain type=%u, falling back to desert garbage\n", __func__, (unsigned)type);
			break;
		}
		/* FALL THROUGH */
	case TerrainAlgorithm::desert_garbage:
	default:
		tgen_desert();
		break;
	}

	// fix tile transitions
}

}
