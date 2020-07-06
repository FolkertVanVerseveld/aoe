/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "graphics.hpp"

#include "imgui/imgui.h"

#include "thread.hpp"

#include <stdexcept>
#include <string>
#include <thread>

namespace genie {

extern std::thread::id t_main;

Tilesheet::Tilesheet(TilesheetBuilder &bld, GLint max) : bnds(), images() {
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

void Tilesheet::write(GLuint tex) {
	assert(is_t_main());

	std::vector<uint32_t> pixels((size_t)((long long)bnds.w * bnds.h));

	// TODO place images
	for (int y = 0; y < bnds.h; ++y)
		for (int x = 0; x < bnds.w; ++x)
			pixels[(long long)y * bnds.w + x] = rand();

	GLuint old_tex;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&old_tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bnds.w, bnds.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glBindTexture(GL_TEXTURE_2D, old_tex);
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

void TilesheetBuilder::sort() noexcept {
	if (!sorted) {
		std::sort(tiles.begin(), tiles.end());
		sorted = true;
	}
}

Texture::Texture(Tilesheet &&ts) : tex(0), ts(ts) {
	assert(genie::is_t_main());
	glGenTextures(1, &tex);
	this->ts.write(tex);
}

Texture::Texture(GLuint tex, Tilesheet &&ts) : tex(tex), ts(ts) {
	if (!tex) {
		assert(genie::is_t_main());
		glGenTextures(1, &tex);
	}
	this->ts.write(tex);
}

Texture::~Texture() {
	if (tex) {
		assert(genie::is_t_main());
		glDeleteTextures(1, &tex);
	}
}

void Texture::bind() {
	assert(tex);
	glBindTexture(GL_TEXTURE_2D, tex);
}

}
