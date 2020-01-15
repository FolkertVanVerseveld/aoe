/**
 * Simple quick and dirty AOE demo using my own cmake-sdl2-template
 *
 * See INSTALL for instructions
 *
 * This is just a quick template to get started. Most code is quickly hacked together and could be made more consistent but I won't bother as long as it fucking works
 */

#include <cctype>
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <stack>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>

#include "os_macros.hpp"
#include "os.hpp"
#include "net.hpp"
#include "engine.hpp"
#include "font.hpp"
#include "menu.hpp"
#include "game.hpp"

#if windows
#pragma comment(lib, "opengl32")
#endif

#define DEFAULT_TITLE "dummy window"

// FIXME move to engine

namespace genie {

class Window final {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> handle;
	std::unique_ptr<Render> renderer;
	Uint32 flags;
public:
	Window(const char *title, int width, int height, Uint32 flags=SDL_WINDOW_SHOWN) : Window(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags) {}

	Window(const char *title, int x, int y, int width, int height, Uint32 flags=SDL_WINDOW_SHOWN, Uint32 rflags=SDL_RENDERER_PRESENTVSYNC) : handle(NULL, &SDL_DestroyWindow), flags(flags) {
		SDL_Window *w;

		if (!(w = SDL_CreateWindow(title, x, y, width, height, flags)))
			throw std::runtime_error(std::string("Unable to create SDL window: ") + SDL_GetError());

		handle.reset(w);

		if (flags & SDL_WINDOW_OPENGL) {
			renderer.reset(new GLRender(w, rflags));
		} else {
			renderer.reset(new SimpleRender(w, rflags));
		}
	}

	SDL_Window* data() { return handle.get(); }

	Render &render() { return *(renderer.get()); }
};

// FIXME move to separate file

class Sfx final {
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> handle;
public:
	Sfx(const char *name) : handle(NULL, &Mix_FreeChunk) {
		Mix_Chunk *c;

		if (!(c = Mix_LoadWAV(name)))
			throw std::runtime_error(std::string("Unable to load sound effect \"") + name + "\" : " + Mix_GetError());

		handle.reset(c);
	}

	int play(int loops=0, int channel=-1) {
		return Mix_PlayChannel(channel, handle.get(), loops);
	}

	Mix_Chunk *data() { return handle.get(); }
};

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

	Config(int argc, char* argv[]);
};

Config::Config(int argc, char* argv[]) : scrmode(ConfigScreenMode::MODE_800_600) {
	unsigned n;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "640")) {
			scrmode = ConfigScreenMode::MODE_640_480;
		}
		else if (!strcmp(argv[i], "800")) {
			scrmode = ConfigScreenMode::MODE_800_600;
		}
		else if (!strcmp(argv[i], "1024")) {
			scrmode = ConfigScreenMode::MODE_1024_768;
		}
		else if (i + 1 < argc && !strcmp(argv[i], "limit")) {
			n = atoi(argv[i + 1]);
			goto set_cap;
		}
		else if (!strcmp(argv[i], "limit=")) {
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

class MenuLobby final : public Menu {
	std::unique_ptr<Multiplayer> mp;
	bool host;
public:
	MenuLobby(SimpleRender& r, Font& f, uint16_t port, bool host = true)
		: Menu(r, f, host ? "Multi Player - Host" : "Multi Player - Client", SDL_Color{ 0xff, 0xff, 0xff })
		, mp(host ? (Multiplayer*)new MultiplayerHost(port) : (Multiplayer*)new MultiplayerClient(port)), host(host) {}

	void keydown(int ch) override {
		switch (ch) {
		case 'q':
		case 'Q':
			nav->quit(1);
			break;
		}
	}
};

class MenuMultiplayer final : public Menu {
	Text txt_host, txt_join, txt_back, txt_port, txt_address;
	TextBuf buf_port, buf_address;
public:
	MenuMultiplayer(SimpleRender& r, Font& f)
		: Menu(r, f, "Multi Player", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_host(r, f, "(H) Host Game", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_join(r, f, "(J) Join Game", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_back(r, f, "(Q) Back", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_port(r, f, "Port: ", SDL_Color{ 0xff, 0xff, 0xff })
		, buf_port(r, f, "25659", SDL_Color{ 0xff, 0xff, 0 })
		, txt_address(r, f, "Server IP: ", SDL_Color{ 0xff, 0xff, 0xff })
		, buf_address(r, f, "127.0.0.1", SDL_Color{ 0xff, 0xff, 0 }) {}

	void keydown(int ch) override {
		if (ch >= '0' && ch <= '9') {
			if (buf_port.str().size() < 5)
				buf_port.append(ch);
			return;
		}

		switch (ch) {
		case SDLK_BACKSPACE:
			buf_port.erase();
			break;
		case 'h':
		case 'H':
			nav->go_to(new MenuLobby(r, f, atoi(buf_port.str().c_str()), true));
			break;
		case 'j':
		case 'J':
			nav->go_to(new MenuLobby(r, f, atoi(buf_port.str().c_str()), false));
			break;
		case 'q':
		case 'Q':
			nav->quit(1);
			break;
		}
	}

	void paint() override {
		Menu::paint();

		txt_host.paint(r, 40, 80);
		txt_join.paint(r, 40, 120);

		txt_port.paint(r, 40, 160);
		buf_port.paint(r, 40 + txt_port.tex().width, 160);

		txt_address.paint(r, 40, 200);
		buf_address.paint(r, 40 + txt_address.tex().width, 200);

		txt_back.paint(r, 40, 240);
	}
};

class MenuStart final : public Menu {
public:
	Text txt_single, txt_multi, txt_quit;

	MenuStart(SimpleRender& r, Font& f)
		: Menu(r, f, "Age of Empires", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_single(r, f, "(S) Single Player", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_multi(r, f, "(M) Multi Player", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_quit(r, f, "(Q) Quit", SDL_Color{ 0xff, 0xff, 0xff }) {}

	void keydown(int ch) override {
		switch (ch) {
		case 's':
		case 'S':
			break;
		case 'm':
		case 'M':
			nav->go_to(new MenuMultiplayer(r, f));
			break;
		case 'q':
		case 'Q':
			nav->quit();
			break;
		}
	}

	void paint() override {
		Menu::paint();

		txt_single.paint(r, 40, 80);
		txt_multi.paint(r, 40, 120);
		txt_quit.paint(r, 40, 160);
	}
};

Navigator::Navigator(SimpleRender& r, Font& f_hdr) : r(r), f_hdr(f_hdr), trace(), top() {
	trace.emplace_back(top = new MenuStart(r, f_hdr));
}

void Navigator::mainloop() {
	while (1) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				if (!top) {
					quit();
					return;
				}

				top->keydown(ev.key.keysym.sym);
				break;
			}
		}

		if (!top) {
			quit();
			return;
		}
		top->idle();

		if (!top) {
			quit();
			return;
		}
		top->paint();

		r.paint();
	}
}

void Navigator::go_to(Menu *m) {
	assert(m);
	trace.emplace_back(top = m);
}

void Navigator::quit(unsigned count) {
	if (!count || count >= trace.size()) {
		trace.clear();
		top = nullptr;
		return;
	}

	trace.erase(trace.end() - count);
	top = trace[trace.size() - 1].get();
}

}

using namespace genie;

int main(int argc, char **argv)
{
	try {
		OS os;
		Config cfg(argc, argv);

		std::cout << "hello " << os.username << " on " << os.compname << '!' << std::endl;

		Engine eng;

#if windows
		char path_fnt[] = "D:\\SYSTEM\\FONTS\\ARIAL.TTF";
		TTF_Font *h_fnt = NULL;

		// find cdrom drive
		for (char dl = 'D'; dl <= 'Z'; ++dl) {
			path_fnt[0] = dl;

			if ((h_fnt = TTF_OpenFont(path_fnt, 13)) != NULL)
				break;
		}
#else
		char path_fnt[] = "/media/cdrom/SYSTEM/FONTS/ARIAL.TTF";
		TTF_Font *h_fnt = TTF_OpenFont(path_fnt, 13);
#endif

		if (!h_fnt) {
			std::cerr << "default font not found" << std::endl;
			return 1;
		}

		// figure out window dimensions
		int width, height;

		switch (cfg.scrmode) {
		case ConfigScreenMode::MODE_640_480: width = 640; height = 480; break;
		case ConfigScreenMode::MODE_1024_768: width = 1024; height = 768; break;
		case ConfigScreenMode::MODE_800_600:
		default:
			width = 800; height = 600; break;
		}

		// run simple demo with SDL_Renderer
		genie::Window w(DEFAULT_TITLE, width, height);

		SimpleRender& r = (SimpleRender&)w.render();

		genie::Font fnt(h_fnt);

		nav.reset(new Navigator(r, fnt));

		nav->mainloop();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
