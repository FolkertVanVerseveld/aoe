/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "graphics.hpp"

#define STB_RECT_PACK_IMPLEMENTATION 1
#define STBRP_LARGE_RECTS 1

#include "imgui/imstb_rectpack.h"

#include "imgui/imgui.h"

#include "math.hpp"
#include "thread.hpp"

#include <stdexcept>
#include <string>
#include <thread>
#include <stack>
#include <map>

namespace genie {

#if STBRP_LARGE_RECTS
constexpr size_t pack_size_max = INT_MAX;
#else
constexpr size_t pack_size_max = SHRT_MAX;
#endif

extern std::thread::id t_main;

Tilesheet::Tilesheet(TilesheetBuilder &bld, GLint max) : bnds(), images(), pixels() {
	bld.sort();

	max = max ? std::min(max, max_texture_size) : max_texture_size;
	// also take into account packer texture limit
	size_t maxsize = std::min<size_t>(prevpow2((size_t)((long long)max * max)), pack_size_max + 1);

	// check if biggest image fits
	auto &tiles = bld.tiles;
	auto &biggest = tiles.back();
	if (biggest.size() > maxsize)
		// splitting images not supported
		throw std::runtime_error("Image too large");

	// TODO use backtracing
	//std::stack<SDL_Rect> backtrack;
	// try to fit as many images as possible
	std::vector<stbrp_rect> buf_rects;
	buf_rects.reserve(tiles.size());
	int fit = 0;

	size_t size;
	for (size = nextpow2(std::max<size_t>(biggest.bnds.w, biggest.bnds.h)); size < maxsize; size <<= 1) {
		images.clear();
		stbrp_context packer = {};
		buf_rects.clear();
		std::vector<stbrp_node> buf_nodes(size);

		::stbrp_init_target(&packer, size, size, buf_nodes.data(), tiles.size());

		// populate tiles
		for (auto it = tiles.rbegin(), end = tiles.rend(); it != end; ++it) {
			auto &img = *it;
			auto &r = buf_rects.emplace_back();
			r.id = img.id;
			r.w = img.bnds.w;
			r.h = img.bnds.h;
			r.was_packed = false;
		}

		if ((fit = ::stbrp_pack_rects(&packer, buf_rects.data(), tiles.size())) != 0)
			break;
	}

	if (!fit)
		throw std::runtime_error("Image too large");

	bnds.w = bnds.h = (int)size;
	pixels.resize(size * size);

#if 0
	// fill unused space with random data
	for (int y = 0; y < bnds.h; ++y)
		for (int x = 0; x < bnds.w; ++x)
			pixels[(long long)y * bnds.w + x] = IM_COL32(rand(), rand(), rand(), rand());
#endif
	std::map<unsigned, unsigned> index;

	for (unsigned i = 0; i < bld.tiles.size(); ++i)
		index.emplace(bld.tiles[i].id, i);

	// place images
	//for (unsigned i = 0; i < images.size(); ++i, tiles.pop_back()) {
	//	auto &t = tiles.back();
	for (auto &t : buf_rects) {
		auto &p = bld.tiles[index.at(t.id)];
		GLfloat s0, t0, s1, t1;

		s0 = (GLfloat)t.x / (GLfloat)size;
		t0 = (GLfloat)t.y / (GLfloat)size;
		s1 = (GLfloat)(t.x + p.bnds.w) / (GLfloat)size;
		t1 = (GLfloat)(t.y + p.bnds.h) / (GLfloat)size;

		images.emplace(0, p.bnds, t.id, t.x, t.y, s0, t0, s1, t1);

		for (int y = 0; y < t.h; ++y)
			for (int x = 0; x < t.w; ++x)
				pixels[((long long)y + t.y) * bnds.w + x + t.x] = p.pixels[(long long)y * t.w + x];
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

void TilesheetBuilder::add(unsigned idx, DrsId id, SDL_Palette *pal) {
	Animation anim(genie::assets.blobs[idx]->open_anim(id));
	for (size_t i = 0; i < anim.size; ++i)
		emplace(anim.images[i], pal);
}

void TilesheetBuilder::sort() noexcept {
	if (!sorted) {
		std::sort(tiles.begin(), tiles.end(), [](const SubImagePreview &lhs, const SubImagePreview &rhs){ return lhs.bnds.h < rhs.bnds.h; });
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
