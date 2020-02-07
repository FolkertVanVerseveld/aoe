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

namespace genie {

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
	MenuLobby(SimpleRender &r, uint16_t port, bool host = true)
		: Menu(MenuId::multiplayer, r, eng->assets->fnt_title, host ? "Multi Player - Host" : "Multi Player - Client", SDL_Color{ 0xff, 0xff, 0xff })
		, mp(host ? (Multiplayer*)new MultiplayerHost(port) : (Multiplayer*)new MultiplayerClient(port))
		, line(r, eng->assets->fnt_default, "", SDL_Color{ 0xff, 0xff, 0 }), host(host) {}

	void idle() override {
		std::lock_guard<std::mutex> lock(mp->mut);
		while (!mp->chats.empty()) {
			chat.emplace_front(r, eng->assets->fnt_default, mp->chats.front().c_str(), SDL_Color{ 0xff, 0xff, 0 });
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

		for (auto &x : chat)
			x.paint(r, 20, 380 - 20 * i++);

		line.paint(20, 400);
	}
};

class MenuMultiplayer final : public Menu {
	Text txt_host, txt_join, txt_back, txt_port, txt_address;
	TextBuf buf_port, buf_address;
public:
	MenuMultiplayer(SimpleRender &r)
		: Menu(MenuId::multiplayer, r, eng->assets->fnt_title, eng->assets->open_str(LangId::title_multiplayer), SDL_Color{ 0xff, 0xff, 0xff })
		, txt_host(r, "(H) Host Game", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_join(r, "(J) Join Game", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_back(r, "(Q) Back", SDL_Color{ 0xff, 0xff, 0xff })
		, txt_port(r, "Port: ", SDL_Color{ 0xff, 0xff, 0xff })
		, buf_port(r, "25659", SDL_Color{ 0xff, 0xff, 0 })
		, txt_address(r, "Server IP: ", SDL_Color{ 0xff, 0xff, 0xff })
		, buf_address(r, "127.0.0.1", SDL_Color{ 0xff, 0xff, 0 }) {}

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
			nav->go_to(new MenuLobby(r, atoi(buf_port.str().c_str()), true));
			break;
		case 'j':
		case 'J':
			nav->go_to(new MenuLobby(r, atoi(buf_port.str().c_str()), false));
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

/** Custom help and game settings menu. */
class MenuExtSettings final : public Menu {
public:
	Text txt_winsize, txt_small, txt_default, txt_medium, txt_fullscreen, txt_back;

	MenuExtSettings(SimpleRender &r)
		: Menu(MenuId::selectnav, r, eng->assets->fnt_title, "Help and Global game settings", SDL_Color{0xff, 0xff,0xff}, true)
		, txt_winsize(r, "Video resolution:", SDL_Color{0xff, 0xff, 0xff})
		, txt_small(r, "(1) 640x480", SDL_Color{0xff, 0xff, 0xff})
		, txt_default(r, "(2) 800x600", SDL_Color{0xff, 0xff, 0xff})
		, txt_medium(r, "(3) 1024x768", SDL_Color{0xff, 0xff, 0xff})
		, txt_fullscreen(r, "(4) Fullscreen", SDL_Color{0xff, 0xff, 0xff})
		, txt_back(r, "(Q) Return to main menu", SDL_Color{0xff, 0xff, 0xff})
		{}

	void keydown(int ch) override {
		switch (ch) {
		case '1':
			eng->w->chmode(ConfigScreenMode::MODE_640_480);
			break;
		case '2':
			eng->w->chmode(ConfigScreenMode::MODE_800_600);
			break;
		case '3':
			eng->w->chmode(ConfigScreenMode::MODE_1024_768);
			break;
		case '4':
			eng->w->chmode(ConfigScreenMode::MODE_FULLSCREEN);
			break;
		case 'q':
		case 'Q':
			nav->quit(1);
			break;
		}
	}

	void paint() override {
		Menu::paint();

		txt_winsize.paint(r, 40, 80);
		txt_small.paint(r, 40, 100);
		txt_default.paint(r, 40, 120);
		txt_medium.paint(r, 40, 140);
		txt_fullscreen.paint(r, 40, 160);
		txt_back.paint(r, 40, 200);
	}
};

class MenuStart final : public Menu {
public:
	Text txt_single, txt_multi, txt_help, txt_quit;

	MenuStart(SimpleRender &r)
		: Menu(MenuId::start, r, eng->assets->fnt_title, eng->assets->open_str(LangId::title_main), SDL_Color{ 0xff, 0xff, 0xff })
		, txt_single(r, "(S) " + eng->assets->open_str(LangId::btn_singleplayer), SDL_Color{ 0xff, 0xff, 0xff })
		, txt_multi(r, "(M) " + eng->assets->open_str(LangId::btn_multiplayer), SDL_Color{ 0xff, 0xff, 0xff })
		, txt_help(r, "(H) Help and settings", SDL_Color{0xff, 0xff, 0xff})
		, txt_quit(r, "(Q) " + eng->assets->open_str(LangId::btn_exit), SDL_Color{ 0xff, 0xff, 0xff }) {}

	void keydown(int ch) override {
		switch (ch) {
		case 's':
		case 'S':
			break;
		case 'm':
		case 'M':
			nav->go_to(new MenuMultiplayer(r));
			break;
		case 'h':
		case 'H':
			nav->go_to(new MenuExtSettings(r));
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
		txt_help.paint(r, 40, 160);
		txt_quit.paint(r, 40, 200);
	}
};

Navigator::Navigator(SimpleRender &r) : r(r), trace(), top() {
	trace.emplace_back(top = new MenuStart(r));
}

}

using namespace genie;

int main(int argc, char **argv)
{
	try {
		Config cfg(argc, argv);

		std::cout << "hello " << os.username << " on " << os.compname << '!' << std::endl;

		Engine eng(cfg);

		SimpleRender &r = (SimpleRender&)eng.w->render();
		nav.reset(new Navigator(r));

		nav->mainloop();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
