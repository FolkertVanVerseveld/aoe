#include "../terrain.hpp"

#include <perlin_noise.hpp>

namespace aoe {

void Terrain::tgen_normal() {
	// TODO stub
	double frequency = 4;
	double octaves = 5;

	const siv::PerlinNoise perlin{ seed };
	const double fx = frequency / this->w;
	const double fy = frequency / this->h;

	for (int32_t y = 0; y < h; ++y) {
		for (int32_t x = 0; x < w; ++x) {
			double v = perlin.octave2D_01(x * fx, y * fx, octaves);

			size_t i = y * w + x;

			if (v > 0.5)
				tiles[i] = Terrain::tile_id(TileType::grass, rand() % 9);
			else if (v > 0.2)
				tiles[i] = Terrain::tile_id(TileType::desert, rand() % 9);
			else
				tiles[i] = Terrain::tile_id(TileType::water, 0);

			// scale 0 to 5
			hmap[i] = (uint8_t)(v * 5);
		}
	}
}

}
