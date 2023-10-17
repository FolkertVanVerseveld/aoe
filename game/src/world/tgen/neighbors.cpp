#include "../../world/terrain.hpp"

namespace aoe {

std::array<unsigned, 8> heights(const std::vector<uint8_t> &hmap, size_t w, size_t h, size_t x, size_t y, unsigned f) {
	std::array<unsigned, 8> n;
	n.fill(f);

	// see comments for next function for neighbor positions

	// 5 6 7
	if (y > 0) {
		if (x > 0)     n[5] = hmap.at((y - 1) * w + x - 1);
		               n[6] = hmap.at((y - 1) * w + x);
		if (x < w - 1) n[7] = hmap.at((y - 1) * w + x + 1);
	}

	// 3 4
	if (x > 0)         n[3] = hmap.at(y * w + x - 1);
	if (x < w - 1)     n[4] = hmap.at(y * w + x + 1);

	// 0 1 2
	if (y < h - 1) {
		if (x > 0)     n[0] = hmap.at((y + 1) * w + x - 1);
		               n[1] = hmap.at((y + 1) * w + x);
		if (x < w - 1) n[2] = hmap.at((y + 1) * w + x + 1);
	}

	return n;
}

std::array<unsigned, 4> hhvn(const std::vector<uint8_t> &hmap, size_t w, size_t h, size_t x, size_t y, unsigned hdef) {
	std::array<unsigned, 4> n;
	n.fill(hdef);

	/*
	  0            (0,  1)
	1 X 2  (-1, 0)         (1, 0)
	  3            (0, -1)
	*/

	if (y > 0)     n[3] = hmap.at((y - 1) * w + x);
	if (x > 0)     n[1] = hmap.at(y * w + x - 1);
	if (x < w - 1) n[2] = hmap.at(y * w + x + 1);
	if (y < h - 1) n[0] = hmap.at((y + 1) * w + x);

	return n;
}

std::array<unsigned, 4> hdn(const std::vector<uint8_t> &hmap, size_t w, size_t h, size_t x, size_t y, unsigned hdef) {
	std::array<unsigned, 4> n;
	n.fill(hdef);

	/*
	0   1  (-1,  1)  (1,  1)
	  X
	2   3  (-1, -1)  (1, -1)
	*/

	if (y > 0) {
		if (x > 0)     n[2] = hmap.at((y - 1) * w + x - 1);
		if (x < w - 1) n[3] = hmap.at((y - 1) * w + x + 1);
	}
	if (y < h - 1) {
		if (x > 0)     n[0] = hmap.at((y + 1) * w + x - 1);
		if (x < w - 1) n[1] = hmap.at((y + 1) * w + x + 1);
	}

	return n;
}

std::array<TileType, 8> neighs(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y, TileType f) {
	std::array<TileType, 8> n;
	n.fill(f);

	/*
	0 1 2  (-1,  1) (0,  1) (1,  1)
	3 X 4  (-1,  0)         (1,  0)
	5 6 7  (-1, -1) (0, -1) (1, -1)
	*/

	// 5 6 7
	if (y > 0) {
		if (x > 0)     n[5] = Terrain::tile_type(tiles.at((y - 1) * w + x - 1));
		               n[6] = Terrain::tile_type(tiles.at((y - 1) * w + x));
		if (x < w - 1) n[7] = Terrain::tile_type(tiles.at((y - 1) * w + x + 1));
	}

	// 3 4
	if (x > 0)         n[3] = Terrain::tile_type(tiles.at(y * w + x - 1));
	if (x < w - 1)     n[4] = Terrain::tile_type(tiles.at(y * w + x + 1));

	// 0 1 2
	if (y < h - 1) {
		if (x > 0)     n[0] = Terrain::tile_type(tiles.at((y + 1) * w + x - 1));
		               n[1] = Terrain::tile_type(tiles.at((y + 1) * w + x));
		if (x < w - 1) n[2] = Terrain::tile_type(tiles.at((y + 1) * w + x + 1));
	}

	return n;
}

/** Find horizontal and vertical neighbors */
std::array<TileType, 4> fhvn(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y, TileType f) {
	std::array<TileType, 4> n;
	n.fill(f);

	/*
	  0            (0,  1)
	1 X 2  (-1, 0)         (1, 0)
	  3            (0, -1)
	*/

	if (y > 0)     n[3] = Terrain::tile_type(tiles.at((y - 1) * w + x));
	if (x > 0)     n[1] = Terrain::tile_type(tiles.at(y * w + x - 1));
	if (x < w - 1) n[2] = Terrain::tile_type(tiles.at(y * w + x + 1));
	if (y < h - 1) n[0] = Terrain::tile_type(tiles.at((y + 1) * w + x));

	return n;
}

/** Like fhvn but no bounds check. Only valid if 0 < x < w - 1, 0 < y < h - 1 */
std::array<TileType, 4> neighs_hv(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y) {
	std::array<TileType, 4> n;

	n[3] = Terrain::tile_type(tiles.at(y - 1) * w + x);
	n[1] = Terrain::tile_type(tiles.at(y * w + x - 1));
	n[2] = Terrain::tile_type(tiles.at(y * w + x + 1));
	n[0] = Terrain::tile_type(tiles.at((y + 1) * w + x));

	return n;
}

/** Find diagonal neighbors */
std::array<TileType, 4> fdn(const std::vector<tile_t> &tiles, size_t w, size_t h, size_t x, size_t y, TileType f) {
	std::array<TileType, 4> n;
	n.fill(f);

	/*
	0   1  (-1,  1)  (1,  1)
	  X
	2   3  (-1, -1)  (1, -1)
	*/

	if (y > 0) {
		if (x > 0)     n[2] = Terrain::tile_type(tiles.at((y - 1) * w + x - 1));
		if (x < w - 1) n[3] = Terrain::tile_type(tiles.at((y - 1) * w + x + 1));
	}
	if (y < h - 1) {
		if (x > 0)     n[0] = Terrain::tile_type(tiles.at((y + 1) * w + x - 1));
		if (x < w - 1) n[1] = Terrain::tile_type(tiles.at((y + 1) * w + x + 1));
	}

	return n;
}

}
