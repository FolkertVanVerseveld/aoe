#pragma once

#include <imgui.h>
#include "../external/imgui_memory_editor.h"

#include <minmax.hpp>

#include <tracy/Tracy.hpp>

#include "engine/gfx.hpp"
#include <vector>

#include "ui.hpp"

#if DEBUG
#define LOGF(f, ...) printf(f, ## __VA_ARGS__)
#define LOG(s) puts(s)
#else
#define LOGF(f, ...) ((void)0)
#define LOG(s) ((void)0)
#endif

namespace aoe {

class Debug final {
	MemoryEditor mem_edit;
	bool show_tm;
	bool ci_ready;

	void show_texture_map();
	void show_display_info(ui::Frame&);
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
