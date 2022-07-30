#pragma once

#include "net.hpp"

#include "sdl.hpp"

#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace aoe {

namespace ui {

class Frame;

}

enum class MenuState {
	init,
	multiplayer_host,
	multiplayer_settings,
	multiplayer_game,
};

class Config final {
public:
	SDL_Rect bnds, display, vp;
	std::string path;

	Config();
	Config(const std::string&);
	~Config();

	void save(const std::string&);
};

struct Resources final {
	int food, wood, gold, stone;

	Resources() : food(0), wood(0), gold(0), stone(0) {}
	Resources(int f, int w, int g, int s) : food(f), wood(w), gold(g), stone(s) {}
};

class PlayerSetting final {
public:
	std::string name;
	bool ai;
	int civ;
	unsigned index; // may overlap
	unsigned team;

	PlayerSetting() : name(), ai(false), civ(0), index(1), team(1) {}
};

class ScenarioSettings final {
public:
	std::vector<PlayerSetting> players;
	bool fixed_start;
	bool explored;
	bool all_technologies;
	bool cheating;
	bool square;
	bool restricted; // allow other players to also change settings
	unsigned width, height;
	unsigned popcap;
	unsigned age;
	unsigned seed;
	unsigned villagers;

	Resources res;

	ScenarioSettings();
};

class Engine final {
	Net net;

	bool show_demo;
	int connection_mode;
	unsigned short connection_port;
	char connection_host[256]; // 253 according to https://web.archive.org/web/20190518124533/https://devblogs.microsoft.com/oldnewthing/?p=7873
	MenuState menu_state;

	// multiplayer_host
	bool multiplayer_ready;
	bool m_show_menubar;

	Config cfg;
	ScenarioSettings scn;
	std::string chat_line;
	std::deque<std::string> chat;
public:
	Engine();
	~Engine();

	int mainloop();
private:
	void display();
	void show_init();
	void show_multiplayer_host();
	void show_mph_tbl(ui::Frame &f);
	void show_mph_cfg(ui::Frame &f);
	void show_menubar();
};

extern Engine *eng;
extern std::mutex m_eng;

}
