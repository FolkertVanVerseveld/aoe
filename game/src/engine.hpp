#pragma once

#include "net.hpp"

#include "async.hpp"
#include "sdl.hpp"
#include "game.hpp"
#include "server.hpp"
#include "ui.hpp"

#include "ctpl_stl.hpp"

#include <queue>
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

enum class EngineAsyncTask {
	server_started = 1 << 0,
	client_connected = 1 << 1,
	multiplayer_stopped = 1 << 2,
	multiplayer_started = 1 << 3,
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

	IdPool<UI_Task> ui_tasks;
	int ui_mod_id;
	std::queue<ui::Popup> popups, popups_async;
	IdPoolRef tsk_start_server;
	std::queue<std::string> chat_async;

	unsigned async_tasks;
	std::atomic<bool> running;
	std::atomic<float> logic_gamespeed;

	bool scroll_to_bottom;
public:
	Engine();
	~Engine();

	int mainloop();
private:
	static constexpr float frame_height = 0.85f, player_height = 0.7f, frame_margin = 0.075f;

	void eventloop(int id);
	void tick();

	void idle();
	void idle_async();

	void display();
	void display_ui_tasks();
	void show_init();

	void show_multiplayer_host();
	void show_mph_tbl(ui::Frame&);
	void show_mph_tbl_footer(ui::Frame &f, bool has_ai);
	void show_mph_cfg(ui::Frame&);
	void show_mph_chat(ui::Frame&);
	void show_chat_line(ui::Frame&);

	void cancel_multiplayer_host();

	void show_multiplayer_game();

	void show_menubar();

	void start_server(uint16_t port);
	void stop_server();
	void stop_server_now(IdPoolRef ref=invalid_ref);

	void start_client(const char *host, uint16_t port);
	void start_client_now(const char *host, uint16_t port);

	void reserve_threads(int n);

	void trigger_async_flags(unsigned f);
	void trigger_async_flags(EngineAsyncTask t) { trigger_async_flags((unsigned)t); }

	void start_multiplayer_game();
public:
	void push_error(const std::string &msg);

	// API for asynchronous tasks
	UI_TaskInfo ui_async(const std::string &title, const std::string &desc, int thread_id, unsigned steps, TaskFlags flags=TaskFlags::all);
	bool ui_async_stop(IdPoolRef);
	bool ui_async_stop(UI_TaskInfo &tsk) { return ui_async_stop(tsk.get_ref()); }

	void ui_async_set_desc(UI_TaskInfo &info, const std::string &s);
	void ui_async_set_total(UI_TaskInfo &info, unsigned total);
	void ui_async_next(UI_TaskInfo &info);
	void ui_async_next(UI_TaskInfo &info, const std::string &s);

	void trigger_server_started();
	void trigger_client_connected();
	void trigger_multiplayer_stop();
	void trigger_multiplayer_started();

	bool is_hosting();

	void add_chat_text(const std::string &s);
};

extern Engine *eng;
extern std::mutex m_eng;

}
