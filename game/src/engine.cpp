// NOTE this header must be included before any OpenGL headers
#include "engine/gfx.hpp"
#include "engine.hpp"

// TODO reorder includes
#include "legacy.hpp"
#include "engine/audio.hpp"
#include "sdl.hpp"

#include <imgui.h>
#include <imgui_impl_sdl.h>
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

#include "imgui_user.hpp"

#include <cstdio>
#include <cstdint>

#include <mutex>
#include <memory>

#include <fstream>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>

#include "debug.hpp"

namespace aoe {

Engine *eng;
std::mutex m_eng;

ScenarioSettings::ScenarioSettings()
	: players(), owners()
	, fixed_start(true), explored(false), all_technologies(false), cheating(false)
	, square(true), wrap(false), restricted(true), reorder(false), width(48), height(48)
	, popcap(100)
	, age(1), seed(1), villagers(3)
	, res(200, 200, 0, 0) {}

Engine::Engine()
	: net(), show_demo(false), show_debug(false), font_scaling(true)
	, connection_mode(0), connection_port(32768), connection_host("")
	, menu_state(MenuState::init), next_menu_state(MenuState::init)
	, multiplayer_ready(false), m_show_menubar(false)
	, chat_line(), chat(), server()
	, tp(2), ui_tasks(), ui_mod_id(), popups(), popups_async()
	, tsk_start_server{ invalid_ref }, chat_async(), async_tasks(0)
	, running(false), logic_gamespeed(1.0f), scroll_to_bottom(false), username(), fd(ImGuiFileBrowserFlags_CloseOnEsc), fd2(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_SelectDirectory), sfx(), music_id(0), music_on(true), game_dir()
	, debug()
	, cfg(*this, "config"), sdl(nullptr), is_fullscreen(false), m_gl(nullptr), assets(), assets_good(false)
	, show_chat(true), show_achievements(false), show_timeline(false), show_diplomacy(false)
	, bkg_vertices{
		// positions          // colors           // texture coords
		 1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 0.0f, // top right
		 1.0f, -1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f, // bottom right
		-1.0f, -1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 1.0f, // bottom left
		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f  // top left
	}, vbo(0), vsync_mode(0), vsync_idx(0)
	, cam_x(0), cam_y(0), keyctl(), gv(), tw(0), th(0), cv()
	, player_tbl_y(0), ui(), fnt()
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
}

gfx::GL &Engine::gl() {
	assert(m_gl.get());
	return *m_gl.get();
}

using namespace aoe::ui;

static const std::vector<std::string> vsync_modes{ "Disabled", "Enabled", "Adaptive" };

void Engine::show_general_settings() {
	music_on = !sfx.is_muted_music();

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

	chkbox("Music enabled", music_on);
	chkbox("Play chat taunts", sfx.play_taunts);
	chkbox("Autostart", cfg.autostart);
	chkbox("Font scaling", font_scaling);

	if (music_on)
		sfx.unmute_music();
	else
		sfx.mute_music();
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
		}
	}
}

/** Load and validate game assets. */
void Engine::verify_game_data(const std::string &path) {
	assets_good = false;

	if (!fnt.loaded())
		return;

	tp.push([this](int id, std::string path) {
		ZoneScoped;
		using namespace io;

		try {
			assets.reset(new Assets(id, *this, path));
			trigger_async_flags(EngineAsyncTask::new_game_data);
		} catch (std::exception &e) {
			fprintf(stderr, "%s: game data verification failed: %s\n", __func__, e.what());
			push_error(std::string("Game data verification failed: ") + e.what());
		}
	}, path);
}

void Engine::display_ui() {
	ZoneScoped;
	show_menubar();

	switch (menu_state) {
		case MenuState::multiplayer_host:
			show_multiplayer_host();
			draw_background_border();
			break;
		case MenuState::multiplayer_game:
			show_terrain();
			show_multiplayer_game();
			break;
		case MenuState::multiplayer_menu:
			show_multiplayer_menu();
			draw_background_border();
			break;
		case MenuState::start:
			show_start();
			draw_background_border();
			break;
		case MenuState::defeat:
			show_defeat();
			draw_background_border();
			break;
		case MenuState::editor_menu:
			ui.show_editor_menu();
			draw_background_border();
			break;
		case MenuState::editor_scenario:
			ui.show_editor_scenario();
			break;
		default:
			show_init();
			break;
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
}

void Engine::display() {
	GLCHK;
	ImGuiIO &io = ImGui::GetIO();
	if (font_scaling)
		io.FontGlobalScale = std::max(1.0f / SDL::fnt_scale, io.DisplaySize.y / SDL::max_h);
	else
		io.FontGlobalScale = 1.0f / SDL::fnt_scale;
	display_ui();
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
		bool cancellable = false;

		for (auto it = eng->ui_tasks.begin(); it != eng->ui_tasks.end(); ++i) {
			UI_Task &tsk = it->second;

			float f = (float)tsk.steps / tsk.total;
			ImGui::TextWrapped("%s", tsk.title.c_str());
			ImGui::ProgressBar(f, ImVec2(-1, 0));
			if (!tsk.desc.empty())
				ImGui::TextWrapped("%s", tsk.desc.c_str());

			std::string str("Cancel##" + std::to_string(i));

			if ((unsigned)tsk.flags & (unsigned)TaskFlags::cancellable)
				cancellable = true;

			if (((unsigned)tsk.flags & (unsigned)TaskFlags::cancellable) && ImGui::Button(str.c_str()))
				it = eng->ui_tasks.erase(it);
			else
				++it;
		}

		if (cancellable && ImGui::Button("Cancel all tasks")) {
			for (auto it = eng->ui_tasks.begin(); it != eng->ui_tasks.end(); ++i) {
				UI_Task &tsk = it->second;
				if ((unsigned)tsk.flags & (unsigned)TaskFlags::cancellable)
					it = eng->ui_tasks.erase(it);
				else
					++it;
			}
		}

		ImGui::EndPopup();
	}
}

void Engine::push_error(const std::string &msg) {
	std::lock_guard<std::mutex> lock(m_async);
	popups_async.emplace(msg, ui::PopupType::error);
}

void Engine::start_client_now(const char *host, uint16_t port) {
	ZoneScoped;

	client.reset(new Client());
	client->start(host, port);
}

void Engine::start_client(const char *host, uint16_t port) {
	ZoneScoped;

	reserve_threads(1);
	tp.push([this](int id, const char *host, uint16_t port) {
		ZoneScoped;

		try {
			UI_TaskInfo info(ui_async("Starting client", "Creating network area", id, 2));

			client.reset(new Client());

			info.next("Connecting to host");

			client->start(host, port);

			trigger_client_connected();
		} catch (std::exception &e) {
			fprintf(stderr, "%s: cannot connect to server: %s\n", __func__, e.what());
			push_error(std::string("cannot connect to server: ") + e.what());
		}
	}, host, port);
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

	reserve_threads(1);
	tp.push([this](int id) {
		std::lock_guard<std::mutex> lk(m_eng);
		if (eng)
			stop_server_now();
	});
}

void Engine::start_server(uint16_t port) {
	ZoneScoped;

	reserve_threads(2);
	tp.push([this](int id, uint16_t port) {
		ZoneScoped;

		try {
			UI_TaskInfo info(ui_async("Starting server", "Creating network area", id, 2));

			// ensures that tsk_start_server is always in a reliable state
			class TskGuard final {
			public:
				bool good;

				TskGuard(UI_TaskInfo &info) : good(false) {
					std::lock_guard<std::mutex> lock(m_eng);
					if (eng)
						eng->stop_server_now(info.get_ref());
				}

				~TskGuard() {
					std::lock_guard<std::mutex> lock(m_eng);
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
				std::lock_guard<std::mutex> lk(m);
				// there should be either no server or an inactive one
				assert(!server || !server->active());
				server.reset(new Server);
			}

			tp.push([this](int id, uint16_t port) {
				server->mainloop(id, port, 1);
			}, port);

			info.next("Connecting to host");

			start_client_now("127.0.0.1", port);

			guard.good = true;
		} catch (std::exception &e) {
			fprintf(stderr, "%s: cannot start server: %s\n", __func__, e.what());
		}
	}, port);
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
	trigger_async_flags(EngineAsyncTask::multiplayer_stopped);
}

void Engine::trigger_multiplayer_started() {
	trigger_async_flags(EngineAsyncTask::multiplayer_started);
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
	//printf("(%.2f,%.2f), (%.2f,%.2f)\n", r.s0, r.t0, r.s1, r.t1);

	bkg_vertices[0 * 8 + 6] = r.s1;
	bkg_vertices[1 * 8 + 6] = r.s1;

	bkg_vertices[2 * 8 + 6] = r.s0;
	bkg_vertices[3 * 8 + 6] = r.s0;

	bkg_vertices[0 * 8 + 7] = r.t0;
	bkg_vertices[3 * 8 + 7] = r.t0;

	bkg_vertices[1 * 8 + 7] = r.t1;
	bkg_vertices[2 * 8 + 7] = r.t1;

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, bkg_vertices.size() * sizeof(GLfloat), bkg_vertices.data(), GL_STATIC_DRAW);

	GLCHK;
}

void Engine::set_background(MenuState s) {
	io::DrsId id = io::DrsId::bkg_main_menu;

	switch (s) {
	case MenuState::multiplayer_host:
	case MenuState::multiplayer_menu:
		id = io::DrsId::bkg_multiplayer;
		break;
	case MenuState::defeat:
		id = io::DrsId::bkg_defeat;
		break;
	case MenuState::editor_menu:
		id = io::DrsId::bkg_editor_menu;
		break;
	}

	set_background(id);
}

ImVec2 Engine::tilepos(int x, int y, float left, float top, int h) {
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

	// load ui cache
	ui.load(*this);
}

void Engine::idle() {
	ZoneScoped;
	idle_async();

	if (menu_state != next_menu_state) {
		menu_state = next_menu_state;
		set_background(menu_state);

		switch (menu_state) {
		case MenuState::start:
			sfx.play_music(MusicId::menu);
			break;
		case MenuState::multiplayer_game:
			sfx.play_music(MusicId::game, -1);
			break;
		case MenuState::editor_scenario:
			sfx.stop_music();
			break;
		case MenuState::defeat:
			sfx.play_music(MusicId::fail);
			break;
		}
	}

	Client *c = client.get();

	if (c)
		cv.try_read(*c);

	if (menu_state == MenuState::multiplayer_game)
		idle_game();
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

	if (dx) cam_x += dx * cam_speed * dt;
	if (dy) cam_y += dy * cam_speed * dt;

	// clamp cam pos
	// TODO clamp right, top and bottom side as well
	cam_x = std::max(0.0f, cam_x);

	std::lock_guard<std::mutex> lk(m);
	if (client)
		gv.try_read(client->g);
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

		if (async_tasks & (unsigned)EngineAsyncTask::multiplayer_stopped)
			cancel_multiplayer_host(MenuState::start);

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
	show_achievements = false;
	show_timeline = false;
	show_diplomacy = false;
	multiplayer_ready = false;
	keyctl.clear();
}

void Engine::add_chat_text(const std::string &s) {
	std::lock_guard<std::mutex> lk(m_async);
	chat_async.emplace(s);
}

void Engine::cancel_multiplayer_host(MenuState next) {
	try {
		if (server) {
			if (!server->active())
				return;

			stop_server();
		} else {
			if (!client->connected())
				return;

			client->stop();
		}

		chat.clear();
		if (menu_state == next_menu_state)
			next_menu_state = next;
	} catch (std::exception &e) {
		fprintf(stderr, "%s: cannot stop multiplayer: %s\n", __func__, e.what());
		push_error(std::string("cannot stop multiplayer: ") + e.what());
	}
}

UI_TaskInfo Engine::ui_async(const std::string &title, const std::string &desc, int thread_id, unsigned steps, TaskFlags flags) {
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
	if (tsk)
		tsk->steps = std::min(tsk->steps + 1u, tsk->total);
	else
		tsk_check_throw(info);
}

void Engine::ui_async_next(UI_TaskInfo &info, const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_ui);
	UI_Task *tsk = ui_tasks.try_get(info.get_ref());
	if (tsk) {
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

int Engine::mainloop() {
	ZoneScoped;

	running = true;

	SDL sdl;
	this->sdl = &sdl;
	vsync_mode = sdl.gl_context.get_vsync();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(sdl.window, sdl.gl_context);
	ImGui_ImplOpenGL3_Init(sdl.guard.glsl_version);

	guess_font_paths();

	// Our state
	ImVec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);

	try {
		cfg.load(cfg.path);
	} catch (const std::runtime_error &e) {
		fprintf(stderr, "%s: could not load config: %s\n", __func__, e.what());
	}

	m_gl.reset(new gfx::GL());

	printf("max texture size: %dx%d\n", m_gl->max_texture_size, m_gl->max_texture_size);

	GLuint vs;

	GLCHK;
	// https://learnopengl.com/Getting-started/Shaders
	vs = glCreateShader(GL_VERTEX_SHADER);

	const GLchar *src;
	GLint length;

	src =
		#include "shaders/shader.vs"
		;

	length = strlen(src);

	glShaderSource(vs, 1, &src, &length);
	glCompileShader(vs);

	GLint status, log_length;

	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &log_length);
		std::string buf(log_length + 1, ' ');
		glGetShaderInfoLog(vs, log_length, NULL, buf.data());
		fprintf(stderr, "%s: vertex shader compile error: %s\n", __func__, buf.c_str());
		return -1;
	}

	GLuint fs;

	fs = glCreateShader(GL_FRAGMENT_SHADER);

	src =
		#include "shaders/shader.fs"
		;

	length = strlen(src);

	glShaderSource(fs, 1, &src, &length);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &log_length);
		std::string buf(log_length + 1, ' ');
		glGetShaderInfoLog(fs, log_length, NULL, buf.data());
		fprintf(stderr, "%s: fragment shader compile error: %s\n", __func__, buf.c_str());
		return -1;
	}

	gfx::GLprogram prog;

	prog += vs;
	prog += fs;

	prog.compile();

	glDeleteShader(fs);
	glDeleteShader(vs);

	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};

	GLuint vao, ebo;

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, bkg_vertices.size() * sizeof(GLfloat), bkg_vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	GLint aPos = glGetAttribLocation(prog, "aPos");
	glVertexAttribPointer(aPos, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(aPos);
	// color attribute
	GLint aColor = glGetAttribLocation(prog, "aColor");
	glVertexAttribPointer(aColor, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(aColor);
	// texture coord attribute
	GLint aTexCoord = glGetAttribLocation(prog, "aTexCoord");
	glVertexAttribPointer(aTexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(aTexCoord);

	glGenTextures(1, &texture1);
	tex1 = (ImTextureID)texture1;
	glBindTexture(GL_TEXTURE_2D, texture1);
		// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLCHK;

	ImageCapture ic(WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);

	// autoload game data if available
	if (!cfg.game_dir.empty())
		verify_game_data(cfg.game_dir);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			bool p;

			switch (event.type) {
				case SDL_QUIT:
					done = true;
					break;
				case SDL_WINDOWEVENT:
					ImGui_ImplSDL2_ProcessEvent(&event);
					if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl.window))
						done = true;
					break;
				case SDL_KEYUP:
					ImGui_ImplSDL2_ProcessEvent(&event);
					keyctl.up(event.key);

					switch (event.key.keysym.sym) {
						case SDLK_BACKQUOTE:
							m_show_menubar = !m_show_menubar;
							break;
						case SDLK_F11:
							sdl.window.set_fullscreen(!sdl.window.is_fullscreen());
							break;
					}
					break;
				case SDL_KEYDOWN:
					p = ImGui_ImplSDL2_ProcessEvent(&event);

					if (!p || (menu_state == MenuState::multiplayer_game && !io.WantCaptureKeyboard))
						keyctl.down(event.key);
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

		GLCHK;

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture1);
		//GLint tex;
		//glGetUniformiv(prog, glGetUniformLocation(prog, "texture1"), &tex);
		prog.use();
		glUniform1i(glGetUniformLocation(prog, "texture1"), 0);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		GLCHK;

		display();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		ic.step(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		SDL_GL_SwapWindow(sdl.window);
		FrameMark;
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	return 0;
}

}
