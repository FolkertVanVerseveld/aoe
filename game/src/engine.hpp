#pragma once

#include "net.hpp"

#include "sdl.hpp"
#include "game.hpp"
#include "server.hpp"

#include "ctpl_stl.hpp"

#include <deque>
#include <mutex>
#include <string>
#include <vector>
#include <condition_variable>

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

class Engine final {
	Net net;

	bool show_demo;
	int connection_mode;
	unsigned short connection_port;
	char connection_host[256]; // 253 according to https://web.archive.org/web/20190518124533/https://devblogs.microsoft.com/oldnewthing/?p=7873
	MenuState menu_state, next_menu_state;

	// multiplayer_host
	bool multiplayer_ready;
	bool m_show_menubar;

	Config cfg;
	ScenarioSettings scn;
	std::string chat_line;
	std::deque<std::string> chat;
	std::mutex m, m_async, m_ui;
	std::unique_ptr<Server> server;
	std::unique_ptr<Client> client;
	std::condition_variable cv_server_start;
	ctpl::thread_pool tp;
public:
	Engine();
	~Engine();

	int mainloop();
private:
	static constexpr float frame_height = 0.85f, player_height = 0.7f, frame_margin = 0.075f;

	void idle();

	void display();
	void show_init();

	void show_multiplayer_host();
	void show_mph_tbl(ui::Frame &f);
	void show_mph_tbl_footer(ui::Frame &f, bool has_ai);
	void show_mph_cfg(ui::Frame &f);
	void show_mph_chat(ui::Frame &f);

	void show_menubar();

	void start_server(uint16_t port);
	void stop_server();

	void start_client(const char *host, uint16_t port);
};

extern Engine *eng;
extern std::mutex m_eng;

}
