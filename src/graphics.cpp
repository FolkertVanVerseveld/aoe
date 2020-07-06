/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "graphics.hpp"

#include "imgui/imgui.h"

#include "math.hpp"
#include "thread.hpp"

#include <stdexcept>
#include <string>
#include <thread>
#include <stack>

namespace genie {

extern std::thread::id t_main;

Tilesheet::Tilesheet(TilesheetBuilder &bld, GLint max) : bnds(), images(), pixels() {
	bld.sort();

	max = max ? std::min(max, max_texture_size) : max_texture_size;
	size_t maxsize = prevpow2((size_t)((long long)max * max));

	// check if biggest image fits
	auto &tiles = bld.tiles;
	auto &biggest = tiles.back();
	if (biggest.size() > maxsize)
		// splitting images not supported
		throw std::runtime_error("Image too large");

	// TODO use backtracing
	//std::stack<SDL_Rect> backtrack;
	// try to fit as many images as possible
	size_t size;
	for (size = nextpow2(std::max<size_t>(biggest.bnds.w, biggest.bnds.h)); size < maxsize; size <<= 1) {
		images.clear();
		bnds.x = bnds.y = 0;

		int next_y = 0;

		// place images
		for (auto it = tiles.rbegin(), end = tiles.rend(); it != end; ++it) {
			auto &img = *it;

			// does it fit
			if ((size_t)((long long)bnds.x + img.bnds.w) > size) {
				// TODO backtrack or start at next row

				// try next row
				if ((size_t)((long long)next_y + img.bnds.h) > size)
					break;

				bnds.x = 0;
				bnds.y = next_y;
			}

			SDL_Rect pos{bnds.x, bnds.y, img.bnds.w, img.bnds.h};
			GLfloat s0, t0, s1, t1;

			next_y = std::max(next_y, bnds.y + img.bnds.h);

			s0 = (GLfloat)pos.x / (GLfloat)size;
			t0 = (GLfloat)pos.y / (GLfloat)size;
			s1 = (GLfloat)(pos.x + pos.w) / (GLfloat)size;
			t1 = (GLfloat)(pos.y + pos.h) / (GLfloat)size;

			images.emplace(0, pos, img.id, img.bnds.x, img.bnds.y, s0, t0, s1, t1);
			bnds.x += img.bnds.w;
		}

		// stop if all tiles fit
		if (images.size() == tiles.size())
			break;
	}

	if (images.empty())
		throw std::runtime_error("Image too large");

	bnds.w = bnds.h = (int)size;
	pixels.resize(size * size);

#if 0
	// fill unused space with random data
	for (int y = 0; y < bnds.h; ++y)
		for (int x = 0; x < bnds.w; ++x)
			pixels[(long long)y * bnds.w + x] = IM_COL32(rand(), rand(), rand(), rand());
#endif

	// place images
	for (unsigned i = 0; i < images.size(); ++i, tiles.pop_back()) {
		auto &t = tiles.back();
		auto &img = *images.find(t.id);

		for (int y = 0; y < t.bnds.h; ++y)
			for (int x = 0; x < t.bnds.w; ++x)
				pixels[((long long)y + img.bnds.y) * bnds.w + x + img.bnds.x] = t.pixels[(long long)y * t.bnds.w + x];
	}
}

void Tilesheet::write(GLuint tex) {
	assert(is_t_main());

	for (auto &img : images)
		img.tex = tex;

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

Texture::Texture() : tex(0), ts() {
	assert(genie::is_t_main());
	glGenTextures(1, &tex);
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
