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

Engine* eng;

Config::Config(int argc, char* argv[]) : scrmode(ConfigScreenMode::MODE_800_600) {
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

Palette Assets::open_pal(uint32_t id) {
	return drs_ui.open_pal(id);
}

Engine::Engine(Config &cfg) : cfg(cfg) {
	assert(!eng);
	eng = this;

	try {
		fs.init(os);
		assets.reset(new Assets());
	} catch (const std::runtime_error &e) {
		show_error(e.what());
		throw;
	}
}

Engine::~Engine() {
	assert(eng);
}

void Engine::show_error(const std::string& title, const std::string &str) {
#if windows
	MessageBox(NULL, str.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
#else
	// yeah i know, sdl message boxes are ugly but linux does not have a standard way to do this that is guaranteed to work...
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), str.c_str(), NULL);
#endif
}

void Engine::show_error(const std::string& str) {
	show_error("Error occurred", str);
}

}