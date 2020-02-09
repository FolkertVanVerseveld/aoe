#include "engine.hpp"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#include "fs.hpp"
#include "os.hpp"
#include "render.hpp"

namespace genie {

Timer::Timer(Uint32 timeout) : ticks(SDL_GetTicks()), remaining(timeout) {}

bool Timer::finished() {
	Uint32 diff, next = SDL_GetTicks();

	if (next < ticks) {
		fputs("timer: overflow\n", stderr);
		diff = 10;
	} else {
		diff = next - ticks;
	}

	ticks = next;
	remaining = diff < remaining ? remaining - diff : 0;

	return remaining == 0;
}

Display::Display(int index) : index(index) {
	if (SDL_GetDisplayBounds(index, &bnds))
		throw std::runtime_error(std::string("Bad default display bounds: ") + SDL_GetError());
}

SDL::SDL() {
	if (SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(std::string("Unable to initialize SDL: ") + SDL_GetError());
}

SDL::~SDL() {
	SDL_Quit();
};

int SDL::display_count() {
	return SDL_GetNumVideoDisplays();
}

void SDL::get_displays(std::vector<Display> &displays) {
	int oldcount = -1, count = -1;
	std::runtime_error last("Bad window manager: unable to determine display count");

	// Since we cannot lock the number of attached displays and we cannot determine if SDL allows devices to be hotswapped, we have to ensure the information we retrieve is consistent
	for (unsigned tries = 3; tries; --tries) {
		count = display_count();
		displays.clear();

		try {
			for (int i = 0; i < count; ++i)
				displays.emplace_back(i);
		} catch (std::runtime_error &e) {
			last = e;
			continue;
		}

		oldcount = count;
		if ((count = display_count()) == oldcount) {
			// prefer displays with higher resolution
			std::sort(displays.begin(), displays.end(), [](const Display &lhs, const Display &rhs) {
				return lhs.bnds.w * lhs.bnds.h > rhs.bnds.w * rhs.bnds.h;
			});
			return;
		}
	}

	throw last;
}

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
	, fnt_button(fs.open_ttf("CoprGtb.ttf", 16))
	, drs_border(fs.open_drs("Border.drs"))
	, drs_gfx(fs.open_drs("graphics.drs"))
	, drs_ui(fs.open_drs("Interfac.drs"))
	, drs_sfx(fs.open_drs("sounds.drs", false))
	, drs_terrain(fs.open_drs("Terrain.drs"))
	, pal_default(open_pal(PAL_DEFAULT))
	, pe_lang(fs.open_pe("language.dll")) {}

Palette Assets::open_pal(res_id id) {
	return drs_ui.open_pal(id);
}

BackgroundSettings Assets::open_bkg(res_id id) {
	return drs_ui.open_bkg(id);
}

Animation Assets::open_slp(const Palette &pal, res_id id) {
	Slp slp;

	if (drs_border.open_slp(slp, id) || drs_gfx.open_slp(slp, id) || drs_ui.open_slp(slp, id) || drs_terrain.open_slp(slp, id))
		return Animation((SimpleRender&)eng->w->render(), pal, slp);

	throw std::runtime_error(std::string("Could not load animation ") + std::to_string(id) + ": bad id");
}

std::string Assets::open_str(LangId id) {
	std::string tmp;
	pe_lang.load_string(tmp, (res_id)id);
	return tmp;
}

Window::Window(const char *title, Config &cfg, Uint32 flags) : Window(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, cfg, flags) {}

Window::Window(const char *title, int x, int y, Config &cfg, Uint32 flags, Uint32 rflags) : handle(NULL, &SDL_DestroyWindow), flags(flags), oldbnds(lgy_dim[0]) {
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

	lastmode = scrmode = cfg.scrmode;

	if (!(w = SDL_CreateWindow(title, x, y, width, height, flags)))
		throw std::runtime_error(std::string("Unable to create SDL window: ") + SDL_GetError());

	handle.reset(w);

	if (flags & SDL_WINDOW_OPENGL)
		renderer.reset(new GLRender(*this, rflags));
	else
		renderer.reset(new SimpleRender(*this, rflags));
}

void Window::chmode(ConfigScreenMode mode) {
	if (scrmode == mode)
		return;

	int index = SDL_GetWindowDisplayIndex(handle.get()), width = 0, height = 0;
	bool was_windowed, go_windowed = true;
	std::vector<Display> displays;
	int d_index = 0;

	switch (mode) {
	case ConfigScreenMode::MODE_640_480: width = 640; height = 480; break;
	case ConfigScreenMode::MODE_1024_768: width = 1024; height = 768; break;
	case ConfigScreenMode::MODE_800_600: width = 800; height = 600; break;
	case ConfigScreenMode::MODE_FULLSCREEN:
		eng->sdl.get_displays(displays);
		assert(!displays.empty());

		// mind you, that the window does not care about aspect ratio: the renderer takes care of that
		go_windowed = false;

		// prefer the currently attached display
		for (int i = 1; i < displays.size(); ++i) {
			if (displays[i].bnds.w != displays[0].bnds.w || displays[i].bnds.h != displays[1].bnds.h)
				break;

			if (index == i) {
				d_index = i;
				break;
			}
		}

		width = displays[d_index].bnds.w;
		height = displays[d_index].bnds.h;
		break;
	}

	assert(height);
	was_windowed = !(SDL_GetWindowFlags(handle.get()) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP));

	if (go_windowed) {
		if (!was_windowed) {
			SDL_SetWindowFullscreen(handle.get(), 0);
			// apparently, we have to force a repaint (at least on windows) to ensure it properly restores the window state
			renderer->paint();
			SDL_SetWindowPosition(handle.get(), oldbnds.x, oldbnds.y);
		}
		SDL_SetWindowSize(handle.get(), width, height);

		renderer->chmode(scrmode, mode);
	} else if (was_windowed) {
		SDL_GetWindowPosition(handle.get(), &oldbnds.x, &oldbnds.y);
		SDL_GetWindowSize(handle.get(), &oldbnds.w, &oldbnds.h);

		SDL_SetWindowPosition(handle.get(), displays[d_index].bnds.x, displays[d_index].bnds.y);
		SDL_SetWindowSize(handle.get(), displays[d_index].bnds.w, displays[d_index].bnds.h);
		// NOTE _DESKTOP is needed for non-primary displays!
		SDL_SetWindowFullscreen(handle.get(), SDL_WINDOW_FULLSCREEN_DESKTOP);

		renderer->chmode(scrmode, mode);
	}

	lastmode = scrmode;
	scrmode = mode;
	// force another repaint to make sure the graphical delay is minimal
	renderer->paint();

	if (!go_windowed) {
		// update render fullscreen dimension
		SDL_Rect &bnds = renderer->dim.lgy_orig;
		scr_dim[(unsigned)ConfigScreenMode::MODE_FULLSCREEN].w = bnds.w;
		scr_dim[(unsigned)ConfigScreenMode::MODE_FULLSCREEN].h = bnds.h;
	}
}

bool Window::toggleFullscreen() {
	switch (scrmode) {
	case ConfigScreenMode::MODE_FULLSCREEN:
		chmode(lastmode == ConfigScreenMode::MODE_FULLSCREEN ? ConfigScreenMode::MODE_800_600 : lastmode);
		return false;
	default:
		chmode(ConfigScreenMode::MODE_FULLSCREEN);
		return true;
	}
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

}