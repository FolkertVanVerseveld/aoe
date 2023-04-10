#include "../game.hpp"

#include <cassert>

#include <tracy/Tracy.hpp>

#include <minmax.hpp>

namespace aoe {

Terrain::Terrain() : tiles(), hmap(), w(0), h(0), seed(0), players(0), wrap(false) {}

void Terrain::resize(unsigned width, unsigned height, unsigned seed, unsigned players, bool wrap) {
	ZoneScoped;

	this->w = width;
	this->h = height;
	this->seed = seed;
	this->players = players;
	this->wrap = wrap;

	tiles.resize(w * h, 0);
	hmap.resize(w * h, 0);
}

void Terrain::generate() {
	// TODO use real generator like perlin noise

	for (size_t i = 0, n = tiles.size(); i < n; ++i) {
		tiles[i] = 1 + rand() % 4;
		hmap[i] = 0;
	}
}

uint8_t Terrain::id_at(unsigned x, unsigned y) {
	return tiles.at(y * w + x);
}

int8_t Terrain::h_at(unsigned x, unsigned y) {
	x = std::clamp(x, 0u, w - 1);
	y = std::clamp(y, 0u, h - 1);
	return hmap.at(y * w + x);
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
