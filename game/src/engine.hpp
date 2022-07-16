#pragma once

#include "net.hpp"

#include "sdl.hpp"

#include <mutex>
#include <string>
#include <vector>

namespace aoe {

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

class PlayerSetting final {
public:
	std::string name;
	unsigned civ;
	unsigned index; // may overlap
	unsigned team;
};

class ScenarioSettings final {
public:
	std::vector<PlayerSetting> players;
	bool fixed_start;
	bool explored;
	bool all_technologies;
	bool cheating;
	unsigned width, height;
	unsigned popcap;

	ScenarioSettings() : players(), fixed_start(true), explored(false), all_technologies(false), cheating(false), width(48), height(48), popcap(100) {}
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

	Config cfg;
	ScenarioSettings scn;
	std::string chat_line;
public:
	Engine();
	~Engine();

	int mainloop();
private:
	void display();
	void show_init();
	void show_multiplayer_host();
	void show_menubar();
};

extern Engine *eng;
extern std::mutex m_eng;

}
