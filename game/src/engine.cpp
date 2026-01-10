// NOTE this header must be included before any OpenGL headers
#include "engine/gfx.hpp"
#include "engine.hpp"

// TODO reorder includes
#include "../legacy/legacy.hpp"
#include "engine/audio.hpp"
#include "engine/sdl.hpp"

#include "ui/fullscreenmenu.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#if _WIN32 || defined(AOE_SDL_NO_PREFIX)
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#else
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif
#endif

#include "ui/imgui_user.hpp"

#include <cstdio>
#include <cstddef>
#include <cstdint>

#include <mutex>
#include <memory>

#include <fstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>

#include "debug.hpp"

#pragma clang diagnostic ignored "-Wswitch"

namespace aoe {

Engine *eng;
std::mutex m_eng;

unsigned sp_player_count = 5, sp_player_ui_count = sp_player_count - 1;
std::array<PlayerSetting, max_legacy_players> sp_players;
ScenarioSettings sp_scn;

MenuState next_menu_state = MenuState::init;

Engine::Engine()
	: net(), show_demo(false), show_debug(false), font_scaling(true)
	, connection_mode(0), connection_port(32768), connection_host("")
	, menu_state(MenuState::init)
	, multiplayer_ready(false), m_show_menubar(false)
	, chat_line(), chat(), server()
	, tp(2), ui_tasks(), ui_mod_id(), popups(), popups_async()
	, tsk_start_server{ invalid_ref }, chat_async(), async_tasks(0)
	, running(false), scroll_to_bottom(false), username(), fd(ImGuiFileBrowserFlags_CloseOnEsc), fd2(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_SelectDirectory)
	, sfx(), music_id(0), music_on(true), music_volume(100.0f), sfx_on(true), sfx_volume(100.0f), game_dir()
	, debug()
	, cfg(*this, "config.ini"), sdl(nullptr), is_fullscreen(false)
#if _WIN32
	, is_clipped(false)
#endif
	, m_gl(nullptr), assets(), assets_good(false)
	, show_chat(false), m_show_achievements(false), show_timeline(false), show_diplomacy(false)
	, vbo(0), vsync_mode(0), vsync_idx(0)
	, cam_x(0), cam_y(0), gv(), tw(0), th(0), cv()
	, player_tbl_y(0), ui()
	, texture1(0), tex1(nullptr)
{
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_eng);
	if (eng)
		throw std::runtime_error("there can be only one");

	eng = this;
}

Engine::~Engine() {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_eng);

	assert(eng);
	eng = nullptr;

	running = false;
	stop_server_now();

	// update config
	cfg.username = username;
}

gfx::GL &Engine::gl() {
	assert(m_gl.get());
	return *m_gl.get();
}

using namespace aoe::ui;

static const std::vector<std::string> vsync_modes{ "Disabled", "Enabled", "Adaptive" };

void Engine::show_general_settings() {
	ZoneScoped;
	int vsync_mode_old = sdl->gl_context.get_vsync();

	switch (vsync_mode_old) {
	case -1: vsync_idx = 2; break;
	case 1: vsync_idx = 1; break;
	default: vsync_idx = 0; break; // 0 and everything else
	}

	combo("VSync", vsync_idx, vsync_modes);

	switch (vsync_idx) {
	case 0: vsync_mode = 0; break;
	case 2: vsync_mode = -1; break;
	default: vsync_mode = 1; break;
	}

	if (vsync_mode != vsync_mode_old)
		sdl->gl_context.set_vsync(vsync_mode);

	music_on = sfx.is_enabled_music();
	if (chkbox("Music enabled", music_on))
		sfx.set_enable_music(music_on);

	const float scale = 100.0;

	music_volume = sfx.get_music_volume() * scale;
	if (ImGui::SliderFloat("Music volume", &music_volume, 0, scale))
		sfx.set_music_volume(music_volume / scale);

	sfx_on = sfx.is_enabled_sound();
	if (chkbox("Sound effects enabled", sfx_on))
		sfx.set_enable_sound(sfx_on);

	sfx_volume = sfx.get_sound_volume() * scale;
	if (ImGui::SliderFloat("Sfx volume", &sfx_volume, 0, scale))
		sfx.set_sound_volume(sfx_volume / scale);

	chkbox("Play chat taunts", sfx.play_taunts);
	chkbox("Autostart", cfg.autostart);
	chkbox("UI scaling", font_scaling);
}

void Engine::show_menubar() {
	ZoneScoped;
	if (!m_show_menubar)
		return;

	MenuBar m;

	if (!m)
		return;

	{
		Menu mf;
		if (mf.begin("File")) {
			{
				Menu ms;

				if (ms.begin("Settings"))
					show_general_settings();
			}

			if (mf.item("Quit"))
				throw 0;
		}
	}

	{
		Menu mv;
		if (mv.begin("View")) {
			mv.chkbox("Demo window", show_demo);
			mv.chkbox("Debug stuff", show_debug);

			bool v = is_fullscreen = sdl->window.is_fullscreen();

			mv.chkbox("Fullscreen", is_fullscreen);

			if (v != is_fullscreen)
				sdl->window.set_fullscreen(is_fullscreen);

#if _WIN32
			v = is_clipped = sdl->window.is_clipped;
			mv.chkbox("Grap mouse", is_clipped);

			if (v != is_clipped)
				sdl->window.set_clipping(is_clipped);
#endif
		}
	}
}

/** Load and validate game assets. */
void Engine::verify_game_data(const std::string &path) {
	if (path.empty())
		return;

	assets_good = false;

	if (!fnt.loaded()) {
		font_scaling = false;
		return;
	}

	std::thread t([this](const char *func, std::string path) {
		ZoneScoped;
		using namespace io;

		try {
			assets.reset(new Assets(*this, path));
			trigger_async_flags(EngineAsyncTask::new_game_data);
		} catch (std::ios_base::failure &e) {
			const char *what = "missing or invalid data";
			if (!path.empty() && path[0] == '/') {
#if _WIN32
				what = "missing data. Game directory looks like a Unix path. Make sure to specify a proper Windows path";
#else
				what = "missing data. Make sure the mount volume is mounted properly";
#endif
			}

			push_error(func, std::string("Game data verification failed: ") + what);
		} catch (std::exception &e) {
			push_error(func, std::string("Game data verification failed: ") + e.what());
		}
	}, __func__, path);

	t.detach();
}

void Engine::start_singleplayer_game() {
	std::thread t([this](const char *func) {
		ZoneScoped;
		using namespace io;

		try {
			ZoneScopedN("starting single player game");
			UI_TaskInfo info(ui_async("Starting single player game", "Initializing player settings", 5));

			LocalClient *lc = new LocalClient();
			{
				lock lk(m);
				assert(!client || !client->connected());
				client.reset(lc);
			}

			lc->event_loop(sp_scn, info);
		} catch (std::exception &e) {
			push_error(func, std::string("Failed to start game: ") + e.what());
		}
	}, __func__);

	t.detach();
}

using namespace gfx;

void Engine::display(gfx::GLprogram &prog, GLuint vao) {
	ZoneScoped;

	GLCHK;
	GL::clearColor(0, 0, 0, 1);

	if (menu_state != MenuState::multiplayer_game) {
		// bind textures on corresponding texture units
		GL::bind2d(0, texture1);

		prog.use();
		prog.setUniform("texture1", 0);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		GLCHK;
	}

	GLCHK;
	ImGuiIO &io = ImGui::GetIO();
	io.FontGlobalScale = 1.0f / SDL::fnt_scale;
	if (font_scaling)
		io.FontGlobalScale = std::max(io.FontGlobalScale, io.DisplaySize.y / SDL::max_h);

	ui.idle(*this);
	show_menubar();

	switch (menu_state) {
	case MenuState::multiplayer_host:
		show_multiplayer_host();
		break;
	case MenuState::multiplayer_game:
	case MenuState::singleplayer_game:
		ui.show_world();
		ui.show_multiplayer_game();
		break;
	case MenuState::singleplayer_menu:
		show_singleplayer_menu();
		break;
	case MenuState::singleplayer_host:
		show_singleplayer_host();
		break;
	case MenuState::multiplayer_menu:
		show_multiplayer_menu();
		break;
	case MenuState::start:
		show_start();
		break;
	case MenuState::defeat:
	case MenuState::victory:
		show_gameover();
		break;
	case MenuState::editor_menu:
		ui.show_editor_menu();
		break;
	case MenuState::editor_scenario:
		ui.show_world();
		ui.show_editor_scenario();
		break;
	case MenuState::init:
		show_init();
		break;
	default:
		throw "invalid menu state";
	}

	const MenuInfo *mi;
	if ((mi = HasMenuInfo(menu_state))) {
		if (mi->draw_border)
			draw_background_border();
	}

	if (show_demo)
		ImGui::ShowDemoWindow(&show_demo);

	if (show_debug)
		debug.show(show_debug);

	display_ui_tasks();

	if (!popups.empty()) {
		ui::Popup &p = popups.front();
		if (!p.show()) {
			ImGui::CloseCurrentPopup();
			popups.pop();
		}
	}

	//io.FontGlobalScale = 1.0f;
	GLCHK;
}

void Engine::display_ui_tasks() {
	ZoneScoped;

	// XXX this is racey but okay ish?
	if (!ui_tasks.size())
		return;

	std::lock_guard<std::mutex> lock(m_ui);

	ImGui::OpenPopup("tasks");
	ImGui::SetNextWindowSize(ImVec2(400, 0));

	if (ImGui::BeginPopupModal("tasks", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		int i = 0;
		unsigned cancellable = 0;

		for (auto it = eng->ui_tasks.begin(); it != eng->ui_tasks.end(); ++i) {
			UI_Task &tsk = it->second;

			float f = (float)tsk.steps / tsk.total;
			DrawTextWrapped(tsk.title);
			ImGui::ProgressBar(f, ImVec2(-1, 0));
			DrawTextWrapped(tsk.desc);

			std::string str("Cancel##" + std::to_string(i));
			if (tsk.is_cancellable())
				++cancellable;

			if (tsk.is_cancellable() && ImGui::Button(str.c_str()))
				it = eng->ui_tasks.erase(it);
			else
				++it;
		}

		// only show when more than one can be cancelled
		if (cancellable > 1 && ImGui::Button("Cancel all tasks")) {
			for (auto it = eng->ui_tasks.begin(); it != eng->ui_tasks.end();) {
				UI_Task &tsk = it->second;
				if (tsk.is_cancellable())
					it = eng->ui_tasks.erase(it);
				else
					++it;
			}
		}

		ImGui::EndPopup();
	}
}

void Engine::push_error(const char *func, const std::string &msg) {
	fprintf(stderr, "%s: %s\n", func, msg.c_str());
	push_error(msg);
}

void Engine::push_error(const std::string &msg) {
	std::lock_guard<std::mutex> lock(m_async);
	popups_async.emplace(msg, ui::PopupType::error);
}

void Engine::push_error(const char *func, SocketError &err) {
	fprintf(stderr, "%s: %s\n", func, err.what());
	push_error(err.user);
}

void Engine::start_client_now(const char *host, uint16_t port, UI_TaskInfo &info) {
	ZoneScoped;

	Client *c = new Client(username);
	client.reset(c);

	puts("Connecting to host");
	info.next("Connecting to host");
	c->start(host, port);
}

void Engine::start_client(const char *host, uint16_t port) {
	ZoneScoped;

	std::thread t([this](const char *func, const char *host, uint16_t port) {
		ZoneScoped;

		try {
			UI_TaskInfo info(ui_async("Starting client", "Creating network area", 2));

			start_client_now(host, port, info);
			trigger_client_connected();
		} catch (SocketError &e) {
			push_error(func, e);
		} catch (std::exception &e) {
			push_error(func, std::string("cannot connect to server: ") + e.what());
		}
	}, __func__, host, port);
	t.detach();
}

void Engine::stop_server_now(IdPoolRef ref) {
	ZoneScoped;

	std::lock_guard<std::mutex> lk(m);
	if (server) {
		server->close();
		tsk_start_server = ref;
	}
}

void Engine::stop_server() {
	ZoneScoped;

	std::thread t([this]() {
		std::lock_guard<std::mutex> lk(m_eng);
		if (eng)
			stop_server_now();
	});
	t.detach();
}

void Engine::start_server(uint16_t port) {
	ZoneScoped;

	std::thread t1([this](const char *func, uint16_t port) {
		ZoneScoped;

		try {
			UI_TaskInfo info(ui_async("Starting server", "Creating network area", 2));

			// ensures that tsk_start_server is always in a reliable state
			class TskGuard final {
			public:
				bool good;

				TskGuard(UI_TaskInfo &info) : good(false) {
					lock lk(m_eng);
					if (eng)
						eng->stop_server_now(info.get_ref());
				}

				~TskGuard() {
					lock lk(m_eng);
					if (!eng)
						return;

					eng->tsk_start_server = invalid_ref;

					if (!good)
						eng->stop_server();
					else
						eng->trigger_server_started();
				}
			} guard(info);

			{
				lock lk(m);
				// there should be either no server or an inactive one
				assert(!server || !server->active());
				server.reset(new Server);
			}

			std::thread t2([this](uint16_t port) {
				((Server*)server.get())->mainloop(port, 1);
			}, port);
			t2.detach();

			server->wait_active(true);
			start_client_now("127.0.0.1", port, info);

			guard.good = true;
		} catch (std::exception &e) {
			fprintf(stderr, "%s: cannot start server: %s\n", func, e.what());
		}
	}, __func__, port);
	t1.detach();
}

void Engine::trigger_async_flags(unsigned f) {
	std::lock_guard<std::mutex> lock(m_async);
	async_tasks |= (unsigned)f;
}

void Engine::trigger_server_started() {
	trigger_async_flags(EngineAsyncTask::server_started);
}

void Engine::trigger_client_connected() {
	trigger_async_flags(EngineAsyncTask::client_connected);
}

void Engine::trigger_multiplayer_stop() {
	trigger_async_flags(EngineAsyncTask::game_stopped);
}

void Engine::trigger_username(const std::string &s) {
	std::lock_guard<std::mutex> lock(m_async);
	username_async = s;
	async_tasks |= (unsigned)EngineAsyncTask::set_username;
}

void Engine::trigger_playermod(const NetPlayerControl &ctl) {
	std::lock_guard<std::mutex> lock(m_async);
	// TODO put in queue
	playermod_async = ctl;
	async_tasks |= (unsigned)EngineAsyncTask::player_mod;
}

bool Engine::is_hosting() {
	std::lock_guard<std::mutex> lk(m);
	return server.get() != nullptr;
}

void Engine::set_background(io::DrsId id) {
	assert(assets.get());
	Assets &a = *assets.get();

	const gfx::ImageRef &r = a.at(id);
	SetBackground(r, vbo);
}

void Engine::set_background(MenuState s) {
	const MenuInfo &mi = GetMenuInfo(s);
	if (mi.border_col != (io::DrsId)0)
		set_background(mi.border_col);
}

ImVec2 Engine::tilepos(float x, float y, float left, float top, int h) {
	float x0 = left + tw / 2 * y + tw / 2 * x;
	float y0 = top  - th / 2 * y + th / 2 * x - th / 2 * h;

	return ImVec2(x0, y0);
}

void Engine::set_game_data() {
	ZoneScoped;

	assert(assets.get());
	Assets &a = *assets.get();
	game_dir = a.path;

	GLuint tex = 1;
	a.ts_ui.write(tex);

	GLCHK;
	set_background(io::DrsId::bkg_main_menu);

	assets_good = true;

	{
		ZoneScopedN("setup cursors");

		if (sdl->cursors.size() > 1)
			sdl->cursors.erase(sdl->cursors.begin() + 1);

		for (unsigned i = 0, n = a.gif_cursors.all_count; i < n; ++i) {
			sdl->cursors.emplace_back(SDL_CreateColorCursor(a.gif_cursors.images[i].surface.get(), a.gif_cursors.images[i].hotspot_x, a.gif_cursors.images[i].hotspot_y), SDL_FreeCursor);
		}
	}

	// set tw and th
	const ImageSet &s_desert = a.anim_at(io::DrsId::trn_desert);
	const gfx::ImageRef &t0 = a.at(s_desert.imgs[0]);
	tw = t0.bnds.w; th = t0.bnds.h;

	ui.load();
}

void Engine::cam_reset() {
	cam_x = cam_y = 0.0f;
}

void Engine::goto_menu(MenuState state) {
	menu_state = state;
	keyctl.clear();
	set_background(menu_state);
	sdl->window.set_clipping(false);

	switch (menu_state) {
	case MenuState::start:
		sfx.play_music(MusicId::menu);
		// pray this never becomes racey
		{
			lock lk(m);
			if (server && !server->active())
				server.reset();
			if (client && !client->connected())
				client.reset();
		}
		break;
	case MenuState::multiplayer_game:
		sdl->window.set_clipping(true);

		sfx.play_music(MusicId::game, -1);
		break;
	case MenuState::editor_scenario:
		sfx.stop_music();
		break;
	case MenuState::defeat:
		sfx.play_music(MusicId::fail);
		break;
	case MenuState::victory:
		sfx.play_music(MusicId::success);
		break;
	}
}

void Engine::idle() {
	ZoneScoped;
	idle_async();

	if (menu_state != next_menu_state)
		goto_menu(next_menu_state);

	IClient *c = client.get();

	if (c)
		cv.try_read(*c);

	if (capture_keys())
		idle_game();
}

bool Engine::capture_keys() const noexcept {
	return menu_state == MenuState::singleplayer_game
		|| menu_state == MenuState::multiplayer_game
		|| menu_state == MenuState::editor_scenario;
}

void Engine::idle_game() {
	ZoneScoped;

	ImGuiIO &io = ImGui::GetIO();

	float dt = io.DeltaTime;
	int dx = 0, dy = 0;

	if (keyctl.is_down(GameKey::key_left )) --dx;
	if (keyctl.is_down(GameKey::key_right)) ++dx;
	if (keyctl.is_down(GameKey::key_up   )) --dy;
	if (keyctl.is_down(GameKey::key_down )) ++dy;

	bool cam_move = dx || dy;

	if (dx) cam_x += dx * cam_speed * dt;
	if (dy) cam_y += dy * cam_speed * dt;

	// clamp cam pos
	// TODO clamp right, top and bottom side as well
	cam_x = std::max(0.0f, cam_x);

	if (menu_state != MenuState::editor_scenario) {
		std::lock_guard<std::mutex> lk(m);
		if (client) {
			IClient *ic = client.get();

			if (cam_move)
				ic->cam_move(cam_x - io.DisplaySize.x / 2, cam_y - io.DisplaySize.y / 2, io.DisplaySize.x, io.DisplaySize.y);

			gv.try_read(ic->g);
		}
	} else {
		std::lock_guard<std::mutex> lk(m);
		ui.idle_editor(*this);
	}
	ui.idle_game();
}

void Engine::idle_async() {
	ZoneScoped;

	std::lock_guard<std::mutex> lock(m_async);

	// copy popups
	for (; !popups_async.empty(); popups_async.pop())
		popups.emplace(popups_async.front());

	if (async_tasks) {
		if (async_tasks & (unsigned)EngineAsyncTask::server_started)
			goto_multiplayer_menu();

		if (async_tasks & (unsigned)EngineAsyncTask::client_connected)
			goto_multiplayer_menu();

		if (async_tasks & (unsigned)EngineAsyncTask::game_stopped)
			quit_game(MenuState::start);

		if (async_tasks & (unsigned)EngineAsyncTask::multiplayer_started)
			start_multiplayer_game();

		if (async_tasks & (unsigned)EngineAsyncTask::set_username)
			username = username_async;

		if (async_tasks & (unsigned)EngineAsyncTask::new_game_data) {
			set_game_data();
			if (cfg.autostart)
				next_menu_state = MenuState::start;
		}
	}

	async_tasks = 0;

	for (; !chat_async.empty(); chat_async.pop())
		chat.emplace_back(chat_async.front());
}

void Engine::goto_multiplayer_menu() {
	next_menu_state = MenuState::multiplayer_host;
	player_tbl_y = 0;
}

void Engine::start_multiplayer_game() {
	ZoneScoped;

	next_menu_state = MenuState::multiplayer_game;
	show_chat = false;
	m_show_achievements = false;
	show_timeline = false;
	show_diplomacy = false;
	multiplayer_ready = false;
	keyctl.clear();

	cam_reset();
}

void Engine::add_chat_text(const std::string &s) {
	std::lock_guard<std::mutex> lk(m_async);
	chat_async.emplace(s);
}

void Engine::quit_game(MenuState next) {
	try {
		// FIXME is this really necessary?
		bool bail = false;

		if (server) {
			if (server->active())
				stop_server();
			else
				bail = true;
		}

		if (client) {
			if (client->connected())
				client->stop();
			else
				bail = true;
		}

		if (bail)
			return;

		chat.clear();
		if (menu_state == next_menu_state)
			next_menu_state = next;
	} catch (std::exception &e) {
		fprintf(stderr, "%s: cannot stop multiplayer: %s\n", __func__, e.what());
		push_error(std::string("cannot stop multiplayer: ") + e.what());
	}
}

UI_TaskInfo Engine::ui_async(const std::string &title, const std::string &desc, unsigned steps, TaskFlags flags) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	auto ref = ui_tasks.emplace(flags, title, desc, 0, steps);
	return UI_TaskInfo(ref.first->first, (TaskFlags)flags);
}

bool Engine::ui_async_stop(IdPoolRef ref) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	return ui_tasks.try_invalidate(ref);
}

/* Throw if task is interruptable. */
static void tsk_check_throw(const UI_TaskInfo &info) {
	if ((unsigned)info.get_flags() & (unsigned)TaskFlags::cancellable)
		throw UI_TaskError("interrupted");
}

void Engine::ui_async_set_desc(UI_TaskInfo &info, const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	UI_Task *tsk = ui_tasks.try_get(info.get_ref());
	if (tsk)
		tsk->desc = s;
	else
		tsk_check_throw(info);
}

void Engine::ui_async_set_total(UI_TaskInfo &info, unsigned total) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	UI_Task *tsk = ui_tasks.try_get(info.get_ref());
	if (tsk)
		tsk->total = total;
	else
		tsk_check_throw(info);
}

void Engine::ui_async_next(UI_TaskInfo &info) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	UI_Task *tsk = ui_tasks.try_get(info.get_ref());
	if (tsk) {
		if (tsk->steps >= tsk->total)
			fprintf(stderr, "ui_taskinfo: too many steps: total=%u\n", tsk->total);
		tsk->steps = std::min(tsk->steps + 1u, tsk->total);
	} else {
		tsk_check_throw(info);
	}
}

void Engine::ui_async_next(UI_TaskInfo &info, const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	UI_Task *tsk = ui_tasks.try_get(info.get_ref());
	if (tsk) {
		if (tsk->steps >= tsk->total)
			fprintf(stderr, "ui_taskinfo: too many steps: total=%u\n", tsk->total);
		tsk->steps = std::min(tsk->steps + 1u, tsk->total);
		tsk->desc = s;
	} else {
		tsk_check_throw(info);
	}
}

void Engine::reserve_threads(int n) {
	if (tp.n_idle() >= n)
		return;

	printf("%s: grow %d\n", __func__, n);
	tp.resize(tp.size() + n);
}

void Engine::guess_font_paths() {
	fnt.try_load();
}

static ImGuiIO &imgui_init(SDL &sdl) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(sdl.window, sdl.gl_context);
	ImGui_ImplOpenGL3_Init(sdl.guard.glsl_version);

	return io;
}

void Engine::cfg_init() {
	ipStatus ret = cfg.load(cfg.path);

	switch (ret) {
	case ipStatus::not_found:
		fprintf(stderr, "%s: no config found\n", __func__);
		return;
	default:
		fprintf(stderr, "%s: i/o error while loading config\n", __func__);
		return;
	case ipStatus::ok:
		break;
	}

	music_volume = cfg.music_volume * 100.0f / SDL_MIX_MAXVOLUME;
	sfx_volume = cfg.sfx_volume * 100.0f / SDL_MIX_MAXVOLUME;

	if (!cfg.username.empty())
		username = cfg.username;
}

static bool shaders_init(GLuint &vs, GLuint &fs) {

	// https://learnopengl.com/Getting-started/Shaders
	vs = GL::createVertexShader();

	const GLchar *src;

	src =
#include "shaders/shader.vs"
		;

	if (GL::compileShader(vs, src) != GL_TRUE) {
		std::string buf(GL::getShaderInfoLog(vs));
		fprintf(stderr, "%s: vertex shader compile error: %s\n", __func__, buf.c_str());
		return false;
	}

	fs = GL::createFragmentShader();

	src =
#include "shaders/shader.fs"
		;

	if (GL::compileShader(fs, src) != GL_TRUE) {
		std::string buf(GL::getShaderInfoLog(vs));
		fprintf(stderr, "%s: fragment shader compile error: %s\n", __func__, buf.c_str());
		return false;
	}

	return true;
}

int Engine::mainloop() {
	ZoneScoped;

	running = true;

	SDL sdl;
	this->sdl = &sdl;
	vsync_mode = sdl.gl_context.get_vsync();

	ImGuiIO &io = imgui_init(sdl);

	cfg_init();
	guess_font_paths();

	m_gl.reset(new GL());

	printf("max texture size: %dx%d\n", m_gl->max_texture_size, m_gl->max_texture_size);

	GLCHK;
	GLuint vs, fs;

	if (!shaders_init(vs, fs))
		return -1;

	GLprogram prog;
	prog.link(vs, fs);

	const unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	GLuint vao, ebo;
	{
		GLvertexArray vao;
		GLbuffer vbo, ebo;
		this->vbo = vbo;

		vao.bind();

		vbo.setData(GL_ARRAY_BUFFER, sizeof(bkg_vertices), bkg_vertices, GL_STATIC_DRAW);
		ebo.setData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		prog.setVertexArray("aPos"     , 3, GL_FLOAT, sizeof(BkgVertex), offsetof(BkgVertex, x));
		prog.setVertexArray("aColor"   , 3, GL_FLOAT, sizeof(BkgVertex), offsetof(BkgVertex, r));
		prog.setVertexArray("aTexCoord", 2, GL_FLOAT, sizeof(BkgVertex), offsetof(BkgVertex, s));

		glGenTextures(1, &texture1);
		tex1 = (ImTextureID)texture1;
		GL::bind2d(texture1, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

		GLCHK;

		// autoload game data if available
		verify_game_data(cfg.game_dir);

		eventloop(sdl, prog, vao);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	return 0;
}

void Engine::kbp_game(GameKey k) {
	ImGuiIO &io = ImGui::GetIO();

	switch (k) {
	case GameKey::toggle_chat:
		show_chat = true;
		break;
	case GameKey::toggle_pause:
		client->send_gamespeed_control(NetGamespeedType::toggle_pause);
		return;
	case GameKey::gamespeed_increase:
		client->send_gamespeed_control(NetGamespeedType::increase);
		return;
	case GameKey::gamespeed_decrease:
		client->send_gamespeed_control(NetGamespeedType::decrease);
		return;
	}

	// focus hotkeys take priority over remaining HUD hotkeys
	switch (k) {
	case GameKey::kill_entity:
		if (!io.WantCaptureMouse)
			ui.try_kill_first_entity();
		return;
	case GameKey::focus_towncenter:
		// TODO focus towncenter if we have any
		if (ui.try_select(EntityType::town_center, cv.playerindex))
			sfx.play_sfx(SfxId::towncenter);
		else
			sfx.play_sfx(SfxId::invalid_select);
		return;
	case GameKey::focus_idle_villager:
		// TODO focus villagers if we have any
		if (ui.find_idle_villager(cv.playerindex))
			sfx.play_sfx(SfxId::villager_random);
		else
			sfx.play_sfx(SfxId::invalid_select);
		return;
	case GameKey::open_buildmenu:
		ui.try_open_build_menu();
		return;
	}

	std::optional<Entity> ent{ ui.first_selected_entity() };

	if (ent.has_value() && ent == cv.playerindex) {
		// if first entity is ours, all selected entities belong to us
		switch (k) {
		case GameKey::train_villager:
			if ((ent = ui.first_selected_building()) == EntityType::town_center)
				client->entity_train(ent->ref, EntityType::villager);
			break;
		case GameKey::train_melee1:
			if ((ent = ui.first_selected_building()) == EntityType::barracks)
				client->entity_train(ent->ref, EntityType::melee1);
			break;
		}
	}
}

void Engine::key_tapped(SDL &sdl, GameKey k) {
	ZoneScoped;

	if (k == GameKey::max)
		return;

	// any menu keys
	switch (k) {
	case GameKey::toggle_debug_window:
		m_show_menubar = !m_show_menubar;
		break;
	case GameKey::open_help:
		open_help();
		break;
	case GameKey::toggle_fullscreen:
		sdl.window.set_fullscreen(!sdl.window.is_fullscreen());
		break;
	}

	// menu specific
	switch (menu_state) {
	case MenuState::singleplayer_game:
	case MenuState::multiplayer_game:
		kbp_game(k);
		break;
	}

	// reset key to prevent 'saving up' key presses
	keyctl.is_tapped(k);
}

void Engine::kbp_down(GameKey k) {
	if (k >= GameKey::max)
		return;

	if (menu_state == MenuState::start)
		mainMenu.kbp_down(k);
}

void Engine::eventloop(SDL &sdl, gfx::GLprogram &prog, GLuint vao) {
	ImageCapture videoRecorder(WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);
	ImGuiIO &io = ImGui::GetIO();

	// Main loop
	bool done = false;
	while (!done) {
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				done = true;
				break;
			case SDL_WINDOWEVENT:
				sdl.window.reclip();
				ImGui_ImplSDL2_ProcessEvent(&event);
				if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl.window))
					done = true;
				break;
			case SDL_KEYUP:
				ImGui_ImplSDL2_ProcessEvent(&event);
				key_tapped(sdl, keyctl.up(event.key));
				break;
			case SDL_KEYDOWN:
				ImGui_ImplSDL2_ProcessEvent(&event);

				if (menu_state == MenuState::start || (capture_keys() && !io.WantCaptureKeyboard))
					kbp_down(keyctl.down(event.key));
				break;
			case SDL_MOUSEBUTTONDOWN:
				ImGui_ImplSDL2_ProcessEvent(&event);

				if (menu_state == MenuState::start) {
					if (event.button.button = SDL_BUTTON_LEFT)
						mainMenu.mouse_down(event.button.x, event.button.y);
				}
				break;
			default:
				ImGui_ImplSDL2_ProcessEvent(&event);
				break;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		idle();
		display(prog, vao);

		// Rendering
		ImGui::Render();
		int w = (int)io.DisplaySize.x, h = (int)io.DisplaySize.y;

		GL::viewport(0, 0, w, h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		videoRecorder.step(0, 0, w, h);
		SDL_GL_SwapWindow(sdl.window);
		FrameMark;
	}
}

}
