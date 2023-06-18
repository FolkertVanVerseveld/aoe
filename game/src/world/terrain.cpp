#include "terrain.hpp"

#include <cassert>

#include <tracy/Tracy.hpp>
#include <minmax.hpp>

#include "../game.hpp"

namespace aoe {

Terrain::Terrain() : tiles(), hmap(), obstructed(false), w(0), h(0), seed(0), players(0), wrap(false) {}

void Terrain::resize(unsigned width, unsigned height, unsigned seed, unsigned players, bool wrap) {
	ZoneScoped;

	this->w = width;
	this->h = height;
	this->seed = seed;
	this->players = players;
	this->wrap = wrap;

	size_t count = (size_t)w * h;

	tiles.resize(count, 0);
	hmap.resize(count, 0);
	obstructed.resize(count, false);
}

void Terrain::generate() {
	// TODO use real generator like perlin noise

	TileType types[] = { TileType::desert, TileType::grass, TileType::grass_desert };

	for (size_t i = 0, n = tiles.size(); i < n; ++i) {
		tiles[i] = Terrain::tile_id(TileType::desert, rand() % 9);
		hmap[i] = 0;
	}
}

uint8_t Terrain::tile_at(unsigned x, unsigned y) {
	return tiles.at(y * w + x);
}

int8_t Terrain::h_at(unsigned x, unsigned y) {
	x = std::clamp(x, 0u, w - 1);
	y = std::clamp(y, 0u, h - 1);
	return hmap.at(y * w + x);
}

void Terrain::add_building(EntityType t, unsigned x, unsigned y) {
	assert(is_building(t));

	// TODO determine size
	const EntityInfo &info = entity_info.at((unsigned)t);

	unsigned x0 = x - info.size / 2, y0 = y - info.size / 2;
	unsigned x1 = x + info.size / 2 + 1, y1 = y + info.size / 2 + 1;

	assert(x0 < x && y0 < y && x1 <= w && y1 < h);

	for (unsigned yy = y0; yy < y1; ++yy) {
		for (unsigned xx = x0; xx < x1; ++xx) {
			size_t pos = yy * w + xx;
			obstructed.at(pos) = true;
		}
	}
}

void Terrain::fetch(std::vector<uint8_t> &tt, std::vector<int8_t> &hm, unsigned x0, unsigned y0, unsigned &w, unsigned &h) {
	tt.clear();
	hm.clear();

	assert(x0 < this->w && y0 < this->h);

	unsigned x1 = std::min(x0 + w, this->w), y1 = std::min(y0 + h, this->h);

	for (unsigned y = y0; y < y1; ++y) {
		for (unsigned x = x0; x < x1; ++x) {
			tt.emplace_back(tiles[y * this->w + x]);
			hm.emplace_back(hmap [y * this->w + x]);
		}
	}

	w = x1 - x0;
	h = y1 - y0;
}

void Terrain::set(const std::vector<uint8_t> &tt, const std::vector<int8_t> &hm, unsigned x0, unsigned y0, unsigned w, unsigned h) {
	assert(x0 < this->w && y0 < this->h);

	unsigned x1 = std::min(x0 + w, this->w), y1 = std::min(y0 + h, this->h);

	for (unsigned y = y0, ty = 0; y < y1; ++y, ++ty) {
		for (unsigned x = x0, tx = 0; x < x1; ++x, ++tx) {
			tiles[y * this->w + x] = tt[ty * w + tx];
			hmap [y * this->w + x] = hm[ty * w + tx];
		}
	}
}

}
