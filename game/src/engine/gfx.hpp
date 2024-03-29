#pragma once

#include <string>
#include <vector>

#include <idpool.hpp>

#include "../legacy/legacy.hpp"

#include "gl.hpp"

#define GLCHK aoe::gfx::glchk(__FILE__, __func__, __LINE__)

namespace aoe {

namespace gfx {

typedef std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> Surface;

void glchk(const char *file, const char *func, int lno);

class Image final {
public:
	Surface surface;
	int hotspot_x, hotspot_y;
	std::vector<std::pair<int,int>> mask;

	Image() : surface(nullptr, SDL_FreeSurface), hotspot_x(0), hotspot_y(0), mask() {}
	Image(const Image&) = delete;
	Image(Image&&) = default;

	bool load(const SDL_Palette *pal, const io::Slp &slp, unsigned index, unsigned player, io::DrsId id);
};

class ImageRef final {
public:
	IdPoolRef ref;
	SDL_Rect bnds;
	int hotspot_x, hotspot_y;
	GLfloat s0, t0, s1, t1;
	SDL_Surface *surf; // NOTE is always NULL if retrieved from Tileset
	std::vector<std::pair<int, int>> mask; // NOTE is empty if mask is rectangular

	ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, int hotspot_x, int hotspot_y);
	ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, const std::vector<std::pair<int,int>> &mask, int hotspot_x, int hotspot_y);
	ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, int hotspot_x, int hotspot_y, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);
	ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, const std::vector<std::pair<int, int>> &mask, int hotspot_x, int hotspot_y, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1);

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
	int w, h;

	Tileset();

	void write(GLuint tex);
};

/** Helper to pack images onto a single big texture image. */
class ImagePacker final {
	IdPool<ImageRef> images;
public:
	ImagePacker();

	IdPoolRef add_img(int hotspot_x, int hotspot_y, SDL_Surface *surf);
	IdPoolRef add_img(int hotspot_x, int hotspot_y, SDL_Surface *surf, const std::vector<std::pair<int,int>> &mask);

	Tileset collect(int w, int h);
};

class GL final {
public:
	GLint max_texture_size;

	GL();

	static GLint getInt(GLenum);

	static GLuint createVertexShader();
	static GLuint createFragmentShader();

	static GLint compileShader(GLuint shader, const char *code);

	static GLint getShaderParam(GLuint shader, GLenum pname);
	static std::string getShaderInfoLog(GLuint shader);
	static GLint getProgramParam(GLuint program, GLenum pname);
	static std::string getProgramInfoLog(GLuint program);
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

	GLprogram &operator-=(GLuint shader) {
		glDeleteShader(shader);
		return *this;
	}

	void link();
	void link(GLuint vs, GLuint fs);

	void use();
};

}
}
