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
#include <vector>

#if _WIN32
#include <Windows.h>
#endif

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
	int get_vsync();

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
#if _WIN32
	RECT old_clip;
	bool is_clipped;
#endif

	Window(const char *title, int x, int y, int w, int h);

	operator SDL_Window*() noexcept { return win.get(); }

	bool is_fullscreen();
	void set_fullscreen(bool v=true);

	void size(int &w, int &h);

	bool set_clipping(bool enable);
};

class SDL final {
public:
	SDLguard guard;
	Window window;
	GLctx gl_context;

	std::vector<std::unique_ptr<SDL_Cursor, decltype(&SDL_FreeCursor)>> cursors;

	static float fnt_scale;
	static int max_h;

	SDL(Uint32 flags=SDL_INIT_VIDEO | SDL_INIT_TIMER);
	SDL(const SDL&) = delete;
	SDL(SDL&&) = delete;

	void set_cursor(unsigned idx);
};

}
