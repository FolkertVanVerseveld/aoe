#pragma once

#include "font.hpp"
#include "drs.hpp"

namespace genie {

//// SDL subsystems ////

class SDL final {
public:
	SDL();
	~SDL();
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

enum class ConfigScreenMode {
	MODE_640_480,
	MODE_800_600,
	MODE_1024_768,
	MODE_FULLSCREEN,
	MODE_CUSTOM,
};

class Config final {
public:
	/** Startup configuration specified by program arguments. */
	ConfigScreenMode scrmode;
	unsigned poplimit = 50;

	static constexpr unsigned POP_MIN = 25, POP_MAX = 200;

	Config(int argc, char *argv[]);
};

#define PAL_DEFAULT 50500

class Assets final {
public:
	Font fnt_default, fnt_title, fnt_button;
	DRS drs_border, drs_gfx, drs_ui, drs_sfx, drs_terrain;
	Palette pal_default;

	Assets();

	Palette open_pal(uint32_t id);
	Background open_bkg(uint32_t id);
};

class Window final {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> handle;
	std::unique_ptr<Render> renderer;
	Uint32 flags;
public:
	Window(const char *title, int width, int height, Uint32 flags = SDL_WINDOW_SHOWN);
	Window(const char *title, int x, int y, int width, int height, Uint32 flags = SDL_WINDOW_SHOWN, Uint32 rflags = SDL_RENDERER_PRESENTVSYNC);

	SDL_Window *data() { return handle.get(); }

	Render &render() { return *(renderer.get()); }
};

class Engine final {
	Config &cfg;
	SDL sdl;
	IMG img;
	MIX mix;
	TTF ttf;
public:
	std::unique_ptr<Assets> assets;
	std::unique_ptr<Window> w;

	Engine(Config &cfg);
	~Engine();

	void show_error(const std::string &title, const std::string &str);
	void show_error(const std::string &str);
};

}
