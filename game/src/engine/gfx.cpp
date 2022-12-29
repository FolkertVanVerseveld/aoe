#include "gfx.hpp"

#include <stdexcept>

namespace aoe {
namespace gfx {

void glchk(const char *file, const char *func, int lno) {
	GLenum err;

	const char *msg = "???";

	switch ((err = glGetError())) {
	case GL_NO_ERROR:
		return;
	case GL_INVALID_ENUM: msg = "invalid enum"; break;
	case GL_INVALID_VALUE: msg = "invalid value"; break;
	case GL_INVALID_OPERATION: msg = "invalid operation"; break;
	case GL_OUT_OF_MEMORY: msg = "out of memory"; break;
	default:
		fprintf(stderr, "%s:%d: %s: unknown OpenGL error 0x%0X\n", file, lno, func, err);
		return;
	}

	fprintf(stderr, "%s:%d: %s: %s\n", file, lno, func, msg);
}

GL::GL() : max_texture_size(0) {
	int ret;

	if ((ret = gl3wInit()) != GL3W_OK)
		throw std::runtime_error(std::string("could not initialize gl3w: code ") + std::to_string(ret));

	max_texture_size = getInt(GL_MAX_TEXTURE_SIZE);
}

GLint GL::getInt(GLenum param) {
	GLint v;
	glGetIntegerv(param, &v);
	return v;
}

GLprogram::GLprogram() : id(glCreateProgram()) {
	if (!id)
		throw std::runtime_error("Failed to create program");
}

GLprogram::~GLprogram() {
	glDeleteProgram(id);
}

void GLprogram::use() {
	glUseProgram(id);
}

void GLprogram::compile() {
	GLint status, log_length;

	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &status);

	if (status == GL_TRUE)
		return;

	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &log_length);
	std::string buf(log_length + 1, ' ');
	glGetProgramInfoLog(id, log_length, NULL, buf.data());

	throw std::runtime_error(std::string("Program compile error: " + buf));
}

}
}
