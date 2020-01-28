#include "engine.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <stdexcept>
#include <string>

#include "fs.hpp"
#include "os.hpp"
#include "render.hpp"

namespace genie {

SDL::SDL() {
	if (SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(std::string("Unable to initialize SDL: ") + SDL_GetError());
}

SDL::~SDL() {
	SDL_Quit();
};

IMG::IMG() {
	if (IMG_Init(0) == -1)
		throw std::runtime_error(std::string("Unable to initialize SDL_Image: ") + IMG_GetError());
}

IMG::~IMG() {
	IMG_Quit();
}

MIX::MIX() {
	int frequency = 44100;
	Uint32 format = MIX_DEFAULT_FORMAT;
	int channels = 2;
	int chunksize = 1024;

	if (Mix_Init(0) == -1)
		throw std::runtime_error(std::string("Unable to initialize SDL_Mixer: ") + Mix_GetError());

	if (Mix_OpenAudio(frequency, format, channels, chunksize))
		throw std::runtime_error(std::string("Unable to open audio device: ") + Mix_GetError());
}

MIX::~MIX() {
	Mix_Quit();
}

TTF::TTF() {
	if (TTF_Init())
		throw std::runtime_error(std::string("Unable to initialize SDL_ttf: ") + TTF_GetError());
}

TTF::~TTF() {
	TTF_Quit();
}

Engine *eng;

Config::Config(int argc, char *argv[]) : scrmode(ConfigScreenMode::MODE_800_600) {
	unsigned n;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "640")) {
			scrmode = ConfigScreenMode::MODE_640_480;
		} else if (!strcmp(argv[i], "800")) {
			scrmode = ConfigScreenMode::MODE_800_600;
		} else if (!strcmp(argv[i], "1024")) {
			scrmode = ConfigScreenMode::MODE_1024_768;
		} else if (i + 1 < argc && !strcmp(argv[i], "limit")) {
			n = atoi(argv[i + 1]);
			goto set_cap;
		} else if (!strcmp(argv[i], "limit=")) {
			n = atoi(argv[i] + strlen("limit="));
		set_cap:
			if (n < POP_MIN)
				n = POP_MIN;
			else if (n > POP_MAX)
				n = POP_MAX;

			poplimit = n;
		}
	}
}

Assets::Assets()
	: fnt_default(fs.open_ttf("Arial.ttf", 12))
	, fnt_title(fs.open_ttf("CoprGtl.ttf", 28))
	, fnt_button(fs.open_ttf("CoprGtl.ttf", 16))
	, drs_border(fs.open_drs("Border.drs"))
	, drs_gfx(fs.open_drs("graphics.drs"))
	, drs_ui(fs.open_drs("Interfac.drs"))
	, drs_sfx(fs.open_drs("sounds.drs", false))
	, drs_terrain(fs.open_drs("Terrain.drs"))
	, pal_default(open_pal(PAL_DEFAULT)) {}

Palette Assets::open_pal(res_id id) {
	return drs_ui.open_pal(id);
}

Background Assets::open_bkg(res_id id) {
	return drs_ui.open_bkg(id);
}

Animation Assets::open_slp(const Palette &pal, res_id id) {
	Slp slp;

	if (drs_border.open_slp(slp, id) || drs_gfx.open_slp(slp, id) || drs_ui.open_slp(slp, id) || drs_terrain.open_slp(slp, id))
		return Animation((SimpleRender&)eng->w->render(), pal, slp);

	throw std::runtime_error(std::string("Could not load animation ") + std::to_string(id) + ": bad id");
}

Window::Window(const char *title, Config &cfg, Uint32 flags) : Window(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg, flags) {}

Window::Window(const char *title, int x, int y, Config &cfg, Uint32 flags, Uint32 rflags) : handle(NULL, &SDL_DestroyWindow), flags(flags) {
	SDL_Window *w;

	// figure out window dimensions
	int width, height;

	switch (cfg.scrmode) {
	case ConfigScreenMode::MODE_640_480: width = 640; height = 480; break;
	case ConfigScreenMode::MODE_1024_768: width = 1024; height = 768; break;
	case ConfigScreenMode::MODE_800_600:
	default:
		width = 800; height = 600; break;
	}

	if (!(w = SDL_CreateWindow(title, x, y, width, height, flags)))
		throw std::runtime_error(std::string("Unable to create SDL window: ") + SDL_GetError());

	handle.reset(w);

	if (flags & SDL_WINDOW_OPENGL)
		renderer.reset(new GLRender(*this, rflags));
	else
		renderer.reset(new SimpleRender(*this, rflags));
}

void Window::chmode(ConfigScreenMode mode) {
	int width, height;

	switch (mode) {
	case ConfigScreenMode::MODE_640_480: width = 640; height = 480; break;
	case ConfigScreenMode::MODE_1024_768: width = 1024; height = 768; break;
	case ConfigScreenMode::MODE_800_600:
	default:
		width = 800; height = 600; break;
	}

	SDL_SetWindowSize(handle.get(), width, height);
	renderer->chmode(mode);
}

#define DEFAULT_TITLE "Age of Empires"

Engine::Engine(Config &cfg) : cfg(cfg) {
	assert(!eng);
	eng = this;

	try {
		fs.init(os);
		assets.reset(new Assets());

		w.reset(new Window(DEFAULT_TITLE, cfg));
	} catch (const std::runtime_error &e) {
		show_error(e.what());
		throw;
	}
}

Engine::~Engine() {
	assert(eng);
}

void Engine::show_error(const std::string &title, const std::string &str) {
#if windows
	MessageBox(NULL, str.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
#else
	// yeah i know, sdl message boxes are ugly but linux does not have a standard way to do this that is guaranteed to work...
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), str.c_str(), NULL);
#endif
}

void Engine::show_error(const std::string &str) {
	show_error("Error occurred", str);
}

void Engine::nextmode() {
	ConfigScreenMode next;

	switch (cfg.scrmode) {
	case ConfigScreenMode::MODE_640_480: next = ConfigScreenMode::MODE_800_600; break;
	case ConfigScreenMode::MODE_800_600: next = ConfigScreenMode::MODE_1024_768; break;
	case ConfigScreenMode::MODE_1024_768:
	default:
		next = ConfigScreenMode::MODE_640_480; break;
	}

	w->chmode(cfg.scrmode = next);
}

}