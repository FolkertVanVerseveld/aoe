#pragma once

#include <GL/gl3w.h>
#include <string>
#include <vector>

#include "../idpool.hpp"
#include "../legacy.hpp"

#define GLCHK aoe::gfx::glchk(__FILE__, __func__, __LINE__)

namespace aoe {

namespace gfx {

typedef std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> Surface;

void glchk(const char *file, const char *func, int lno);

class Image final {
public:
	Surface surface;
	int hotspot_x, hotspot_y;

	Image() : surface(nullptr, SDL_FreeSurface), hotspot_x(0), hotspot_y(0) {}
	Image(const Image&) = delete;
	Image(Image&&) = default;

	bool load(const SDL_Palette *pal, const io::Slp &slp, unsigned index, unsigned player=0);
};

class ImageRef final {
public:
	IdPoolRef ref;
	SDL_Rect bnds;
	GLfloat s0, t0, s1, t1;
	SDL_Surface *surf; // NOTE is always NULL if retrieved from Tileset

	ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf=NULL);
	ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);

	friend bool operator<(const ImageRef &lhs, const ImageRef &rhs) noexcept {
		return lhs.ref < rhs.ref;
	}
};

/** Big texture that contains all references images. */
class Tileset final {
public:
	std::set<ImageRef> imgs;
	/** Raw surface where images are blitted to. Note that after write(GLuint), this will become undefined! */
	Surface surf;

	Tileset();

	void write(GLuint tex);
};

/** Helper to pack images onto a single big texture image. */
class ImagePacker final {
	IdPool<ImageRef> images;
public:
	ImagePacker();

	IdPoolRef add_img(SDL_Surface *surf);

	Tileset collect(int w, int h);
};

class GL final {
public:
	GLint max_texture_size;

	GL();

	static GLint getInt(GLenum);
};

class GLprogram final {
	GLuint id;
public:
	GLprogram();
	~GLprogram();

	operator GLuint() const noexcept { return id; }

	GLprogram &operator+=(GLuint shader) {
		glAttachShader(id, shader);
		return *this;
	}

	void compile();

	void use();
};

}
}
