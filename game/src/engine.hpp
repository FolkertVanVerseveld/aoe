#pragma once

#include "net.hpp"

#include "async.hpp"
#include "engine/audio.hpp"
#include "sdl.hpp"
#include "game.hpp"
#include "server.hpp"
#include "ui.hpp"

#include "ctpl_stl.hpp"

#include "debug.hpp"
#include "engine/assets.hpp"

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
	start,
	multiplayer_menu,
	multiplayer_host,
	multiplayer_settings,
	multiplayer_game,
	defeat,
};

class Engine;

class Config final {
	Engine &e;
public:
	SDL_Rect bnds, display, vp;
	std::string path, game_dir;

	static constexpr uint32_t magic = 0x06ce09f6;

	Config(Engine&);
	Config(Engine&, const std::string&);
	~Config();

	void load(const std::string&);
	void save(const std::string&);
};

enum class EngineAsyncTask {
	server_started = 1 << 0,
	client_connected = 1 << 1,
	multiplayer_stopped = 1 << 2,
	multiplayer_started = 1 << 3,
	set_scn_vars = 1 << 4,
	set_username = 1 << 5,
	player_mod = 1 << 6,
	new_game_data = 1 << 7,
};

class EngineView;

// TODO make engine view that wraps eng and m_eng magic
// TODO split async?
// TODO split ui
class Engine final {
	Net net;

	bool show_demo, show_debug;
	int connection_mode;
	unsigned short connection_port;
	char connection_host[256]; // 253 according to https://web.archive.org/web/20190518124533/https://devblogs.microsoft.com/oldnewthing/?p=7873
	MenuState menu_state, next_menu_state;

	// multiplayer_host
	bool multiplayer_ready;
	bool m_show_menubar;

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
	ScenarioSettings scn_async;

	unsigned async_tasks;
	std::atomic<bool> running;
	std::atomic<float> logic_gamespeed;

	bool scroll_to_bottom;

	std::string username;
	std::string username_async;

	NetPlayerControl playermod_async;

	ImGui::FileBrowser fd, fd2;
public:
	Audio sfx;
private:
	int music_id;
	bool music_on;

	std::string game_dir;

	Debug debug;
	Config cfg;

	SDL *sdl;
	bool is_fullscreen;
	std::unique_ptr<gfx::GL> m_gl;

	std::unique_ptr<Assets> assets;
	bool assets_good;
	// TODO move this to a game class or smth
	bool show_achievements;
	bool show_timeline;
	bool show_diplomacy;

	std::vector<GLfloat> bkg_vertices;
	GLuint vbo;
	int vsync_mode, vsync_idx;

	friend Debug;
	friend Config;
	friend EngineView;
public:
	Engine();
	~Engine();

	int mainloop();

	gfx::GL &gl();
private:
	static constexpr float frame_height = 0.9f, player_height = 0.55f, frame_margin = 0.075f;

	void eventloop(int id);
	void tick();

	void idle();
	void idle_async();

	void verify_game_data(const std::string &path);
	void set_game_data();

	void display();
	void display_ui();
	void display_ui_tasks();
	void show_init();
	void show_start();
	void show_defeat();

	void show_multiplayer_menu();
	// TODO extract as HUD
	void show_multiplayer_host();
	void show_mph_tbl(ui::Frame&);
	void show_mph_cfg(ui::Frame&);
	void show_mph_chat(ui::Frame&);
	void show_chat_line(ui::Frame&);

	void cancel_multiplayer_host(MenuState next);

	void show_multiplayer_game();
	void show_multiplayer_achievements();
	void show_multiplayer_diplomacy();

	void show_general_settings();
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

	void set_scn_vars_now(const ScenarioSettings &scn);
	void playermod(const NetPlayerControl&);

	void set_background(MenuState);
	void set_background(io::DrsId);

	void draw_background_border();
	void guess_font_paths();
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
	void trigger_username(const std::string&);
	void trigger_playermod(const NetPlayerControl&);

	bool is_hosting();

	void add_chat_text(const std::string &s);

	void set_scn_vars(const ScenarioSettings &scn);
};

extern Engine *eng;
extern std::mutex m_eng;

// TODO use this
class EngineView final {
	std::lock_guard<std::mutex> lk;
public:
	EngineView();

	void play_sfx(SfxId id, int loops=0);
};

}
