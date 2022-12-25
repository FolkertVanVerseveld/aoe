#pragma once

#include <GL/gl3w.h>
#include <string>
#include <vector>

#define GLCHK aoe::gfx::glchk(__FILE__, __func__, __LINE__)

namespace aoe {
namespace gfx {


void glchk(const char *file, const char *func, int lno);

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
