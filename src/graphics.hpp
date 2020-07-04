/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <cstdint>
#include <vector>
#include <algorithm>

#include <GL/gl3w.h>

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
	GLuint tex;
	SDL_Rect bnds; // x,y are top & left. w,h are size
	int hotx, hoty;
	GLfloat s0, t0, s1, t1;

	SubImage(GLuint tex, const SDL_Rect &bnds, int hotx, int hoty, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1)
		: tex(tex), bnds(bnds), hotx(hotx), hoty(hoty), s0(s0), t0(t0), s1(s1), t1(t1) {}
};

class TextureBuilder;

class Texture final {
public:
	GLuint tex;
	SDL_Rect bnds; // x,y are cursor for next texture to place. w,h are bounds
	std::vector<SubImage> images;

	Texture() : tex(0), bnds(), images() {}

	/**
	 * Munch all SubImagePreviews from TextureBuilder and limit texture size to \a max.
	 * If max is zero, OpenGL's max texture size is used.
	 * Note: bld.tiles may not be empty.
	 */
	Texture(TextureBuilder&, GLint max=0);
	Texture(const Texture&) = delete;
	Texture(Texture&&) = default;

	~Texture();
};

/** Single image used for texture builder. */
class SubImagePreview final {
public:
	std::vector<uint32_t> pixels; // RGBA
	SDL_Rect bnds; // x,y are hotspot x and y. w,h are size
	unsigned id; // for texture and texturebuilder to keep track of reordered images

	SubImagePreview(const Image &img, const SDL_Palette *pal) : SubImagePreview(0, img, pal) {}
	SubImagePreview(unsigned id, const Image &img, const SDL_Palette *pal);

	constexpr size_t size() const noexcept { return (size_t)((long long)bnds.w * bnds.h); }

	friend bool operator<(const SubImagePreview &lhs, const SubImagePreview &rhs) { return lhs.size() < rhs.size(); }
};

class TextureBuilder final {
public:
	std::vector<SubImagePreview> tiles;
	int w, h;
	size_t minarea;
	bool sorted;

	TextureBuilder() : tiles(), w(0), h(0), minarea(0), sorted(false) {}

	template<typename... Args>
	decltype(auto) emplace(Args&&... args) {
		auto t = tiles.emplace_back(tiles.size(), args...);
		w = std::max(w, t.bnds.w);
		h = std::max(h, t.bnds.h);
		minarea += t.bnds.w * t.bnds.h;
		sorted = false;
		return t;
	}

	void sort() noexcept;
};

}
