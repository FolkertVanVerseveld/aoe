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

class MenuLobby final : public Menu {
	std::unique_ptr<Multiplayer> mp;
	TextBuf line;
	bool host;
	std::deque<Text> chat;
public:
	MenuLobby(SimpleRender& r, Font& f, uint16_t port, bool host = true)
		: Menu(r, f, host ? "Multi Player - Host" : "Multi Player - Client", SDL_Color{ 0xff, 0xff, 0xff })
		, mp(host ? (Multiplayer*)new MultiplayerHost(port) : (Multiplayer*)new MultiplayerClient(port))
		, line(r, f, "", SDL_Color{ 0xff, 0xff, 0 }), host(host) {}

	void idle() override {
		std::lock_guard<std::mutex> lock(mp->mut);
		while (!mp->chats.empty()) {
			chat.emplace_front(r, f, mp->chats.front().c_str(), SDL_Color{ 0xff, 0xff, 0 });
			mp->chats.pop();
		}
	}

	void keydown(int ch) override {
		if (ch != '\n' && ch != '\r' && ch <= 0xff && isprint((unsigned char)ch)) {
			line.append(ch);
			return;
		}

		switch (ch) {
		case SDLK_BACKSPACE:
			line.erase();
			break;
		case '\n':
		case '\r':
			{
				auto s = line.str();
				if (!s.empty()) {
					if (s == "/clear")
						chat.clear();
					else
						mp->chat(s);
					line.clear();
				}
				break;
			}
		case SDLK_ESCAPE:
			nav->quit(1);
			break;
		}
	}

	void paint() override {
		Menu::paint();

		unsigned i = 0;

		for (auto& x : chat)
			x.paint(r, 20, 380 - 20 * i++);

		line.paint(20, 400);
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
		case SDLK_ESCAPE:
			nav->quit(1);
			break;
		}
	}

	void paint() override {
		Menu::paint();

		txt_host.paint(r, 40, 80);
		txt_join.paint(r, 40, 120);

		txt_port.paint(r, 40, 160);
		buf_port.paint(40 + txt_port.tex().width, 160);

		txt_address.paint(r, 40, 200);
		buf_address.paint(40 + txt_address.tex().width, 200);

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
		case SDLK_ESCAPE:
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
		Config cfg(argc, argv);

		std::cout << "hello " << os.username << " on " << os.compname << '!' << std::endl;

		Engine eng(cfg);

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

		nav.reset(new Navigator(r, eng.assets->fnt_default));

		nav->mainloop();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
