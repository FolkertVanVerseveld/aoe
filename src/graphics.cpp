/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "graphics.hpp"

#include "imgui/imgui.h"

#include "thread.hpp"

#include <stdexcept>
#include <string>
#include <thread>

namespace genie {

extern std::thread::id t_main;

Texture::Texture(TextureBuilder &bld, GLint max) : tex(0), bnds(), images() {
	bld.sort();

	if (!max)
		max = max_texture_size;

	size_t maxsize = (size_t)((long long)max * max);

	// check if biggest image fits
	auto &tiles = bld.tiles;
	auto &biggest = tiles.back();
	if (biggest.size() > maxsize)
		// splitting images not supported
		throw std::runtime_error("Image too large");

	// place all images on first row
	bnds.h = biggest.bnds.h;

	for (auto it = tiles.rbegin(), end = tiles.rend(); it != end; ++it) {
		auto &img = *it;
		bnds.w += img.bnds.w;
	}
}

Texture::~Texture() {
	if (tex) {
		// OpenGL calls are only allowed in main thread
		assert(is_t_main());
		glDeleteTextures(1, &tex);
	}
}

SubImagePreview::SubImagePreview(unsigned id, const Image &img, const SDL_Palette *pal) : pixels(img.bnds.w * img.bnds.h), bnds(img.bnds), id(id) {
	if (img.bnds.w <= 0 || img.bnds.h <= 0)
		throw std::runtime_error("invalid image boundaries");

	// TODO support SDL_Surface
	const std::vector<uint8_t> &data = std::get<std::vector<uint8_t>>(img.surf);

	// convert
	for (int y = 0; y < bnds.h; ++y)
		for (int x = 0; x < bnds.w; ++x) {
			unsigned long long pos = (unsigned long long)y * bnds.w + x;
			SDL_Color *col = &pal->colors[data[pos]];
			pixels[pos] = IM_COL32(col->r, col->g, col->b, data[pos] ? col->a : 0);
		}
}

void TextureBuilder::sort() noexcept {
	if (!sorted) {
		std::sort(tiles.begin(), tiles.end());
		sorted = true;
	}
}

}
