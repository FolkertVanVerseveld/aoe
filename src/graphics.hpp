/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <cstdint>
#include <vector>

#include <GL/gl3w.h>

#include "drs.hpp"

namespace genie {

/** Single image in texture */
class SubImage final {
public:
	SDL_Rect bnds;
	int hotx, hoty;
	GLfloat s0, t0, s1, t1;

	SubImage(const SDL_Rect &bnds, int hotx, int hoty, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1)
		: bnds(bnds), hotx(hotx), hoty(hoty), s0(s0), t0(t0), s1(s1), t1(t1) {}
};

class TextureBuilder;

class Texture final {
public:
	GLuint tex;
	SDL_Rect bnds;
	std::vector<SubImage> images;

	Texture();
	Texture(TextureBuilder&);
	Texture(const Texture&) = delete;
	Texture(Texture&&) = default;

	~Texture();
};

/** Single image used for texture builder. */
class SubImagePreview final {
public:
	std::vector<uint32_t> pixels; // RGBA
	SDL_Rect bnds; // x,y are hotspot x and y. w,h are size

	SubImagePreview(const Image &img, const SDL_Palette *pal);

	constexpr size_t size() const noexcept { return (size_t)((long long)bnds.w * bnds.h); }

	friend bool operator<(const SubImagePreview &lhs, const SubImagePreview &rhs) {
		return lhs.size() < rhs.size();
	}
};

class TextureBuilder final {
public:
	std::vector<SubImagePreview> tiles;
	bool sorted;

	TextureBuilder();

	template<typename... Args>
	void emplace(Args&&... args) {
		tiles.emplace_back(args...);
	}

	void sort();
};

}
