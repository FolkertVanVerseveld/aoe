#include "sdl.hpp"

#include <stdexcept>
#include <string>

#include <tracy/Tracy.hpp>

#include "../legacy/legacy.hpp"

namespace aoe {

SDLguard::SDLguard(Uint32 flags) : flags(flags)
#if defined(IMGUI_IMPL_OPENGL_ES2)
	, glsl_version("#version 100")
#elif defined(__APPLE__)
	, glsl_version("#version 150")
#else
	, glsl_version("#version 130")
#endif
{
	ZoneScoped;
	if (SDL_Init(flags))
		throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

SDLguard::~SDLguard() {
	ZoneScoped;

	SDL_Quit();
}

float SDL::fnt_scale = 1.0f;
int SDL::max_h = WINDOW_HEIGHT_MIN;

SDL::SDL(Uint32 flags) : guard(flags)
	, window("Age of Empires", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN)
	, gl_context(window), cursors()
{
	SDL_SetWindowMinimumSize(window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);
	gl_context.enable(window);
	gl_context.set_vsync(1);

	cursors.emplace_back(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW), SDL_FreeCursor);

	SDL_SetCursor(cursors.front().get());

	// find tallest display
	int count = SDL_GetNumVideoDisplays();

	int href = 768, hmax = href;

	for (int i = 0; i < count; ++i) {
		SDL_Rect bnds;
		SDL_GetDisplayBounds(i, &bnds);

		hmax = std::max(hmax, bnds.h);
	}

	fnt_scale = (float)hmax / href;
	max_h = hmax;
}

void SDL::set_cursor(unsigned idx) {
	const auto &c = cursors.at(idx);
	SDL_SetCursor(c.get());
}

Window::Window(const char *title, int x, int y, int w, int h)
#pragma warning(disable: 26812)
	: flags((SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI))
#pragma warning(default: 26812)
	, win(SDL_CreateWindow(title, x, y, w, h, flags), SDL_DestroyWindow)
	, mode_def{ 0 }, mode_full{ 0 }, pos_def{ 0 }, disp_full(-1) {}

GLctx::GLctx(SDL_Window *win) : gl_context(SDL_GL_CreateContext(win)) {}

GLctx::~GLctx() {
	SDL_GL_DeleteContext(gl_context);
}

void GLctx::enable(SDL_Window *win) {
	SDL_GL_MakeCurrent(win, gl_context);
}

void GLctx::set_vsync(int mode) {
	SDL_GL_SetSwapInterval(mode);
}

int GLctx::get_vsync() {
	return SDL_GL_GetSwapInterval();
}

bool Window::is_fullscreen() {
	return (SDL_GetWindowFlags(win.get()) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::set_fullscreen(bool v) {
	if (v == is_fullscreen())
		return;

	SDL_Window *w = win.get();

	if (v) {
		disp_full = SDL_GetWindowDisplayIndex(w);

		SDL_GetWindowDisplayMode(w, &mode_def);
		SDL_GetCurrentDisplayMode(disp_full, &mode_full);

		SDL_GetWindowPosition(w, &pos_def.x, &pos_def.y);
		SDL_GetWindowSize(w, &pos_def.w, &pos_def.h);
		SDL_GetDisplayBounds(disp_full, &pos_full);

		SDL_SetWindowPosition(w, pos_full.x, pos_full.y);
		SDL_SetWindowSize(w, pos_full.w, pos_full.h);

		SDL_SetWindowDisplayMode(w, &mode_full);
		SDL_SetWindowFullscreen(w, SDL_WINDOW_FULLSCREEN);
	} else {
		SDL_SetWindowFullscreen(w, 0);
		SDL_SetWindowSize(w, pos_def.w, pos_def.h);
		SDL_SetWindowPosition(w, pos_def.x, pos_def.y);
	}
}

void Window::size(int &w, int &h) {
	SDL_GetWindowSize(win.get(), &w, &h);
}

}
