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

GLuint GL::createVertexShader() {
	return glCreateShader(GL_VERTEX_SHADER);
}

GLuint GL::createFragmentShader() {
	return glCreateShader(GL_FRAGMENT_SHADER);
}

GLint GL::compileShader(GLuint shader, const char *code) {
	GLint length = (GLint)strlen(code);

	glShaderSource(shader, 1, (const GLchar**)&code, &length);
	glCompileShader(shader);

	return getShaderParam(shader, GL_COMPILE_STATUS);
}

GLint GL::getShaderParam(GLuint shader, GLenum param) {
	GLint v;
	glGetShaderiv(shader, param, &v);
	return v;
}

std::string GL::getShaderInfoLog(GLuint shader) {
	GLint log_length = getShaderParam(shader, GL_INFO_LOG_LENGTH);
	std::string s(log_length + 1, ' ');
	glGetShaderInfoLog(shader, log_length, NULL, s.data());
	return s;
}

GLint GL::getProgramParam(GLuint program, GLenum param) {
	GLint v;
	glGetProgramiv(program, param, &v);
	return v;
}

std::string GL::getProgramInfoLog(GLuint program) {
	GLint log_length = getProgramParam(program, GL_INFO_LOG_LENGTH);
	std::string s(log_length + 1, ' ');
	glGetProgramInfoLog(program, log_length, NULL, s.data());
	return s;
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

void GLprogram::link() {
	glLinkProgram(id);

	if (GL::getProgramParam(id, GL_LINK_STATUS) == GL_TRUE)
		return;

	std::string buf(GL::getProgramInfoLog(id));
	throw std::runtime_error(std::string("Program compile error: " + buf));
}

void GLprogram::link(GLuint vs, GLuint fs) {
	*this += vs;
	*this += fs;

	link();

	*this -= fs;
	*this -= vs;
}

}
}
