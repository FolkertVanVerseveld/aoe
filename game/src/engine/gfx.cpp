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

void GL::bind2d(int idx, GLuint tex) {
	glActiveTexture(GL_TEXTURE0 + idx);
	glBindTexture(GL_TEXTURE_2D, tex);
}

void GL::bind2d(GLuint tex, GLint wrapS, GLint wrapT, GLint minFilter, GLint magFilter) {
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
}

void GL::clearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GL::viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
	glViewport(x, y, width, height);
}

GLvertexArray::GLvertexArray() : id(0) {
	glGenVertexArrays(1, &id);
}

GLvertexArray::~GLvertexArray() {
	glDeleteVertexArrays(1, &id);
}

void GLvertexArray::bind() {
	glBindVertexArray(id);
}

GLbuffer::GLbuffer() : id(0) {
	glGenBuffers(1, &id);
}

GLbuffer::~GLbuffer() {
	glDeleteBuffers(1, &id);
}

void GLbuffer::setData(GLenum target, GLsizeiptr size, const void *data, GLenum usage) {
	glBindBuffer(target, id);
	glBufferData(target, size, data, usage);
}

GLprogram::GLprogram() : id(glCreateProgram()), uniforms(), attributes() {
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

void GLprogram::setUniform(const char *name, GLint v) {
	auto it = uniforms.find(name);
	GLint id = -1;

	if (it != uniforms.end()) {
		id = it->second;
	} else {
		id = glGetUniformLocation(this->id, name);

		if (id == -1)
			return;

		uniforms.emplace(name, id);
	}

	glUniform1i(id, v);
}

void GLprogram::setVertexArray(const char *name, GLint size, GLenum type, GLsizei stride, unsigned offset) {
	setVertexArray(name, size, type, GL_FALSE, stride, offset);
}

void GLprogram::setVertexArray(const char *name, GLint size, GLenum type, GLboolean normalized, GLsizei stride, unsigned offset) {
	auto it = attributes.find(name);
	GLint id = -1;

	if (it != attributes.end()) {
		id = it->second;
	} else {
		id = glGetAttribLocation(this->id, name);

		if (id == -1)
			return;

		attributes.emplace(name, id);
	}

	glVertexAttribPointer(id, size, type, normalized, stride, (void*)(0 + offset));
	glEnableVertexAttribArray(id);
}

}
}
