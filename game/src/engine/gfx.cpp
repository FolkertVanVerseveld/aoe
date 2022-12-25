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

GL::GL() {
	int ret;

	if ((ret = gl3wInit()) != GL3W_OK)
		throw std::runtime_error(std::string("could not initialize gl3w: code ") + std::to_string(ret));
}

GLbuffer::GLbuffer() : id(0) {
	glGenBuffers(1, &id);
	GLCHK;
}

GLbuffer::~GLbuffer() {
	glDeleteBuffers(1, &id);
}

GLbufferview::GLbufferview(GLbuffer &b) : b(b) {
	glBindBuffer(GL_ARRAY_BUFFER, b.id);
	GLCHK;
}

GLbufferview::~GLbufferview() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLbufferview::draw(const GLvoid *data, GLsizeiptr size) {
	glBufferData(GL_ARRAY_BUFFER, size, data, GL_STREAM_DRAW);
}

GLshader::GLshader(GLenum type) : id(glCreateShader(type)), type(type) {
	if (!id)
		throw std::runtime_error("could not create shader");
}

GLshader::~GLshader() {
	glDeleteShader(id);
}

void GLshader::build() {
	std::vector<const GLchar*> src;

	for (const std::string &s : lines)
		src.emplace_back(s.c_str());

	glShaderSource(id, src.size(), src.data(), NULL);
	glCompileShader(id);

	GLint status = 0, log_length = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &log_length);

	if (status != GL_TRUE) {
		if (log_length > 1) {
			std::string buf(log_length + 1, ' ');
			glGetShaderInfoLog(id, log_length, NULL, buf.data());
			fprintf(stderr, "%s: %s\n", __func__, buf.c_str());
		}

		throw std::runtime_error("failed to compile shader");
	}
}

GLprogram::GLprogram() : id(glCreateProgram()) {
	if (!id)
		throw std::runtime_error("could not create program");
}

GLprogram::~GLprogram() {
	glDeleteProgram(id);
}

void GLprogram::build() {
	GLint status = 0, log_length = 0;

	glLinkProgram(id);
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	
	if (status != GL_TRUE) {
		if (log_length > 1) {
			std::string buf(log_length + 1, ' ');
			glGetShaderInfoLog(id, log_length, NULL, buf.data());
			fprintf(stderr, "%s: %s\n", __func__, buf.c_str());
		}

		throw std::runtime_error("failed to compile program");
	}
}

}
}
