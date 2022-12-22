#pragma once

#include <imgui.h>
#include "imgui_memory_editor.h"

#include "nominmax.hpp"

#include <tracy/Tracy.hpp>

#include <GL/gl3w.h>
#include <vector>

namespace aoe {

class Debug final {
	MemoryEditor mem_edit;
public:
	void show(bool &open);
};

class ImageCapture final {
#if TRACY_ENABLE
	GLuint m_fiTexture[4];
	GLuint m_fiFramebuffer[4];
	GLuint m_fiPbo[4];
	GLsync m_fiFence[4];
	int m_fiIdx;
	std::vector<int> m_fiQueue;
	GLsizei w, h;
public:
	ImageCapture(GLsizei, GLsizei);

	void step(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1);
#else
public:
	ImageCapture(GLsizei, GLsizei) {}
	void step(GLint, GLint, GLint, GLint) {}
#endif
};

}
