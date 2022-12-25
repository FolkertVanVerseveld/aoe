#pragma once

#include <GL/gl3w.h>
#include <string>
#include <vector>

#include "../legacy.hpp"

#define GLCHK aoe::gfx::glchk(__FILE__, __func__, __LINE__)

namespace aoe {

namespace gfx {

void glchk(const char *file, const char *func, int lno);

class Image final {
public:
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface;
	int hotspot_x, hotspot_y;

	Image() : surface(nullptr, SDL_FreeSurface), hotspot_x(0), hotspot_y(0) {}
	Image(const Image&) = delete;
	Image(Image&&) = default;

	bool load(const SDL_Palette *pal, const io::Slp &slp, unsigned index, unsigned player=0);
};

// TODO repurpose or remove everything below this line

class GL final {
public:
	GL();
};

class GLprogram;

class GLshader final {
	GLuint id;
	GLenum type;
	std::vector<std::string> lines;
	friend GLprogram;
public:
	GLshader(GLenum type);
	~GLshader();

	GLshader &operator+=(const std::string &s) {
		lines.emplace_back(s);
		return *this;
	}

	void build();
};

class GLbufferview;

class GLbuffer final {
	GLuint id;
	friend GLbufferview;
public:
	GLbuffer();
	~GLbuffer();
};

class GLbufferview final {
	GLbuffer &b;
public:
	GLbufferview(GLbuffer &b);
	~GLbufferview();

	GLbufferview(const GLbuffer&) = delete;
	GLbufferview(GLbuffer&&) = delete;

	void draw(const GLvoid *data, GLsizeiptr size);
};

class GLprogram final {
public:
	GLuint id;

	GLprogram();
	~GLprogram();

	GLprogram &operator+=(const GLshader &s) {
		glAttachShader(id, s.id);
		return *this;
	}

	void build();
};

}
}
