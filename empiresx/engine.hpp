#pragma once

#include "cfg.hpp"
#include "font.hpp"
#include "drs.hpp"
#include "pe.hpp"
#include "lang.hpp"

#include <vector>
#include <string>

namespace genie {

//// SDL subsystems ////

class Timer final {
	Uint32 ticks, remaining;
public:
	Timer(Uint32 timeout);

	bool finished();
};

// FIXME use display to determine best windowed and fullscreen mode
class Display final {
public:
	int index;
	SDL_Rect bnds;

	Display(int index=0);
	Display(int index, const SDL_Rect &bnds) : index(index), bnds(bnds) {}
};

class SDL final {
public:
	SDL();
	~SDL();

	int display_count();
	/** Determine all attached video monitors and their dimensions. */
	void get_displays(std::vector<Display> &displays);
};

class IMG final {
public:
	IMG();
	~IMG();
};

class MIX final {
public:
	MIX();
	~MIX();
};

//// Engine stuff ////

class Engine;

extern Engine *eng;

#define PAL_DEFAULT 50500

class Assets final {
public:
	Font fnt_default, fnt_title, fnt_button;
	DRS drs_border, drs_gfx, drs_ui, drs_sfx, drs_terrain;
	Palette pal_default;
	PE pe_lang;

	Assets();

	Palette open_pal(res_id id);
	BackgroundSettings open_bkg(res_id id);
	Animation open_slp(const Palette &pal, res_id id);
	std::string open_str(LangId id);
};

class Window final {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> handle;
	std::unique_ptr<Render> renderer;
	Uint32 flags;
	SDL_Rect oldbnds;
	ConfigScreenMode lastmode, scrmode;
public:
	Window(const char *title, Config &cfg, Uint32 flags = SDL_WINDOW_SHOWN);
	Window(const char *title, int x, int y, Config &cfg, Uint32 flags = SDL_WINDOW_SHOWN, Uint32 rflags = SDL_RENDERER_PRESENTVSYNC);

	SDL_Window *data() { return handle.get(); }

	Render &render() { return *(renderer.get()); }
	ConfigScreenMode mode() const { return scrmode; }
	/** Change window resolution. */
	void chmode(ConfigScreenMode mode);
	/** Alternate between windowed and fullscreen mode and return true if new mode is fullscreen. */
	bool toggleFullscreen();
};

class Engine final {
	Config &cfg;
public:
	SDL sdl;
	IMG img;
	MIX mix;
	TTF ttf;
	std::unique_ptr<Assets> assets;
	std::unique_ptr<Window> w;

	Engine(Config &cfg);
	~Engine();

	void show_error(const std::string &title, const std::string &str);
	void show_error(const std::string &str);
};

}
