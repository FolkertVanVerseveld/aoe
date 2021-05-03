/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <memory>
#include <set>

#include <SDL2/SDL_opengl.h>
//#include <GL/gl3w.h>

#include "drs.hpp"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace genie {

extern GLint max_texture_size;

/** Single image in texture */
class SubImage final {
public:
	mutable GLuint tex; // just for convenience. Texture keeps track of tex's lifecycle.
	SDL_Rect bnds; // x,y are hotspot. w,h are size
	unsigned id; // for tilesheet to keep track of reordered images
	GLfloat s0, t0, s1, t1;
	int tx, ty; // real position on texture

	std::vector<bool> mask; // if not opaque, this contains collision mask

	SubImage(unsigned id) : tex(0), bnds(), id(id), s0(0), t0(0), s1(0), t1(0), mask() {}

	SubImage(GLuint tex, int tx, int ty, const SDL_Rect &bnds, unsigned id, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1, const std::vector<bool> &mask)
		: tex(tex), tx(tx), ty(ty), bnds(bnds), id(id), s0(s0), t0(t0), s1(s1), t1(t1), mask(mask) {}

	/** Pixel perfect collision detection. */
	bool contains(int px, int py) const;

	friend bool operator<(const SubImage &lhs, unsigned id) { return lhs.id < id; }
	friend bool operator<(const SubImage &lhs, const SubImage &rhs) { return lhs.id < rhs.id; }
};

class TilesheetBuilder;

class Strip final {
public:
	unsigned id, w, h, count;
	std::vector<unsigned> tiles;
	bool dynamic;

	Strip(unsigned id) : id(id), w(0), h(0), count(0), tiles(), dynamic(false) {}

	friend bool operator<(const Strip &lhs, unsigned id) { return lhs.id < id; }
	friend bool operator<(const Strip &lhs, const Strip &rhs) { return lhs.id < rhs.id; }
};

class Tilesheet final {
public:
	SDL_Rect bnds; // x,y are cursor for next texture to place. w,h are bounds
	std::set<SubImage> images;
	// we could have used vector, but if the ids are global and other tilesheets are linked, this will break
	std::set<Strip> animations;
	std::vector<uint32_t> pixels; // RGBA

	Tilesheet() : bnds(), images(), animations(), pixels() {}

	/**
	 * Munch all SubImagePreviews from TextureBuilder and limit texture size to \a max.
	 * If max is zero, OpenGL's max texture size is used.
	 * Note: bld.tiles may not be empty.
	 */
	Tilesheet(TilesheetBuilder&, GLint max=0);

	/** Dump contents to OpenGL texture. This must be run on the OpenGL thread. */
	void write(GLuint tex);

	const SubImage &operator[](unsigned id) const { return *images.find(id); }
};

/** Single image used for texture builder. */
class SubImagePreview final {
public:
	std::vector<uint32_t> pixels; // RGBA
	SDL_Rect bnds; // x,y are hotspot x and y. w,h are size
	unsigned id; // for texture and texturebuilder to keep track of reordered images
	std::vector<bool> mask; // if not fully filled, this contains collision mask

	SubImagePreview(const Image &img, const SDL_Palette *pal) : SubImagePreview(0, img, pal) {}
	SubImagePreview(unsigned id, const Image &img, const SDL_Palette *pal);

	constexpr size_t size() const noexcept { return (size_t)((long long)bnds.w * bnds.h); }

	friend bool operator<(const SubImagePreview &lhs, const SubImagePreview &rhs) { return lhs.size() < rhs.size(); }
};

class TilesheetBuilder final {
public:
	std::vector<SubImagePreview> tiles;
	std::set<Strip> animations;
	int w, h;
	size_t minarea;
	bool sorted;

	TilesheetBuilder() : tiles(), animations(), w(0), h(0), minarea(0), sorted(false) {}

	template<typename... Args>
	decltype(auto) emplace(Args&&... args) {
		auto t = tiles.emplace_back(tiles.size(), args...);
		w = std::max(w, t.bnds.w);
		h = std::max(h, t.bnds.h);
		minarea += (size_t)((long long)t.bnds.w * t.bnds.h);
		sorted = false;
		return t;
	}

	unsigned add(unsigned idx, DrsId id, SDL_Palette *pal);
	void sort() noexcept;
};

class Texture final {
public:
	GLuint tex;
	Tilesheet ts;

	Texture();

	Texture(const Texture&) = delete;
	Texture(Texture &&t) noexcept : tex(t.tex), ts(t.ts) {}

	~Texture();

	void bind();
};

}
