#pragma once

#pragma warning(push)
#pragma warning(disable: 26812)
#pragma warning(disable: 26819)

// SDL2 is installed differently per OS
#if _WIN32 || defined(AOE_SDL_NO_PREFIX)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#pragma warning(pop)

#include <memory>

namespace aoe {

class SDLguard final {
public:
	Uint32 flags;
	const char *glsl_version;

	SDLguard(Uint32 flags);
	~SDLguard();
};

class GLctx final {
public:
	SDL_GLContext gl_context;

	GLctx(SDL_Window *win);
	~GLctx();

	void enable(SDL_Window *win);
	void set_vsync(int mode);

	operator SDL_GLContext() noexcept { return gl_context; }
};

class Window final {
public:
	SDL_WindowFlags flags;
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> win;

	// def=default (windowed mode), full=fullscreen
	SDL_DisplayMode mode_def, mode_full;
	SDL_Rect pos_def, pos_full;
	int disp_full;

	Window(const char *title, int x, int y, int w, int h);

	operator SDL_Window*() noexcept { return win.get(); }

	bool is_fullscreen();
	void set_fullscreen(bool v=true);
};

class SDL final {
public:
	SDLguard guard;
	Window window;
	GLctx gl_context;

	SDL(Uint32 flags=SDL_INIT_VIDEO | SDL_INIT_TIMER);
	~SDL();
};

}
