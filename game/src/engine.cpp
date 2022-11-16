#include "engine.hpp"

#include "legacy.hpp"
#include "sdl.hpp"
#include "string.hpp"

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

#include "nominmax.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

Engine *eng;
std::mutex m_eng;

static const char *connection_modes[] = {"host game", "join game"};

static void write(std::ofstream &out, SDL_Rect r) {
	static_assert(sizeof(int) == sizeof(int32_t));
	out.write((const char *)&r.x, sizeof r.x);
	out.write((const char *)&r.y, sizeof r.y);
	out.write((const char *)&r.w, sizeof r.w);
	out.write((const char *)&r.h, sizeof r.h);
}

Config::Config() : Config("") {}
Config::Config(const std::string &s) : bnds{ 0, 0, 1, 1 }, display{ 0, 0, 1, 1 }, vp{ 0, 0, 1, 1 }, path(s) {}

Config::~Config() {
	if (path.empty())
		return;

	try {
		save(path);
	} catch (std::exception&) {}
}

void Config::save(const std::string &path) {
	ZoneScoped;
	std::ofstream out(path, std::ios_base::binary | std::ios_base::trunc);

	// let c++ take care of any errors
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	uint32_t magic = 0x06ce09f6;

	out.write((const char*)&magic, sizeof magic);
	write(out, bnds);
	write(out, display);
	write(out, vp);
}

ScenarioSettings::ScenarioSettings()
	: players(8)
	, fixed_start(true), explored(false), all_technologies(false), cheating(false)
	, square(true), restricted(true), reorder(false), hosting(false), width(48), height(48)
	, popcap(100)
	, age(1), seed(1), villagers(3)
	, res(200, 200, 0, 0) {}

Engine::Engine()
	: net(), show_demo(false)
	, connection_mode(0), connection_port(32768), connection_host("")
	, menu_state(MenuState::init), next_menu_state(MenuState::init)
	, multiplayer_ready(false), m_show_menubar(true)
	, cfg("config"), scn()
	, chat_line(), chat(), server()
	, tp(2), ui_tasks(), ui_mod_id(), popups(), popups_async()
	, tsk_start_server{ invalid_ref }, chat_async(), async_tasks(0), running(false), logic_gamespeed(1.0f), scroll_to_bottom(false)
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

void Engine::show_menubar() {
	ZoneScoped;
	if (!m_show_menubar || !ImGui::BeginMainMenuBar())
		return;

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("Quit"))
			throw 0;

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View")) {
		ImGui::Checkbox("Demo window", &show_demo);
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void Engine::show_init() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	if (!ImGui::Begin("init", NULL, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse)) {
		ImGui::End();
		return;
	}

	ImGui::SetWindowSize(vp->WorkSize);

	//ImGui::TextUnformatted("welcome to the free software age of empires setup launcher");

	ImGui::Combo("connection mode", &connection_mode, connection_modes, IM_ARRAYSIZE(connection_modes));

	if (connection_mode == 1) {
		ImGui::InputText("host", connection_host, sizeof(connection_host));
		ImGui::SameLine();
		if (ImGui::Button("localhost"))
			strncpy0(connection_host, "127.0.0.1", sizeof(connection_host));
	}

	ImGui::InputScalar("port", ImGuiDataType_U16, &connection_port);

	if (ImGui::Button("start")) {
		switch (connection_mode) {
			case 0:
				start_server(connection_port);
				break;
			case 1:
#if 0
				try {
					start_client(connection_host, connection_port);
					scn.hosting = false;
					next_menu_state = MenuState::multiplayer_host;
				} catch (std::exception &e) {
					fprintf(stderr, "%s: cannot join server: %s\n", __func__, e.what());
					push_error(std::string("cannot join server: ") + e.what());
				}
#else
				start_client(connection_host, connection_port);
#endif
				break;
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("quit"))
		throw 0;

	ImGui::End();
}

void Engine::display() {
	ZoneScoped;
	show_menubar();

	switch (menu_state) {
		case MenuState::multiplayer_host:
			show_multiplayer_host();
			break;
		case MenuState::multiplayer_game:
			show_multiplayer_game();
			break;
		default:
			show_init();
			break;
	}

	if (show_demo)
		ImGui::ShowDemoWindow(&show_demo);

	display_ui_tasks();

	if (!popups.empty()) {
		ui::Popup &p = popups.front();
		if (!p.show()) {
			ImGui::CloseCurrentPopup();
			popups.pop();
		}
	}
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

bool Engine::is_hosting() {
	std::lock_guard<std::mutex> lk(m);
	return server.get() != nullptr;
}

void Engine::idle() {
	ZoneScoped;
	idle_async();
	menu_state = next_menu_state;
}

void Engine::idle_async() {
	ZoneScoped;

	std::lock_guard<std::mutex> lock(m_async);

	// copy popups
	for (; !popups_async.empty(); popups_async.pop())
		popups.emplace(popups_async.front());

	if (async_tasks) {
		if (async_tasks & (unsigned)EngineAsyncTask::server_started) {
			scn.hosting = true;
			next_menu_state = MenuState::multiplayer_host;
		}

		if (async_tasks & (unsigned)EngineAsyncTask::client_connected) {
			scn.hosting = false;
			next_menu_state = MenuState::multiplayer_host;
		}

		if (async_tasks & (unsigned)EngineAsyncTask::multiplayer_stopped)
			cancel_multiplayer_host();

		if (async_tasks & (unsigned)EngineAsyncTask::multiplayer_started)
			start_multiplayer_game();
	}

	async_tasks = 0;

	for (; !chat_async.empty(); chat_async.pop())
		chat.emplace_back(chat_async.front());
}

void Engine::start_multiplayer_game() {
	ZoneScoped;

	next_menu_state = MenuState::multiplayer_game;
}

void Engine::add_chat_text(const std::string &s) {
	std::lock_guard<std::mutex> lk(m_async);
	chat_async.emplace(s);
}

void Engine::cancel_multiplayer_host() {
	try {
		if (server)
			stop_server();
		else
			client->stop();

		next_menu_state = MenuState::init;
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

UI_TaskInfo::~UI_TaskInfo() {
	ZoneScoped;
	// just tell engine task has completed, we don't care if it succeeds
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		(void)eng->ui_async_stop(*this);
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

void UI_TaskInfo::set_total(unsigned total) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_set_total(*this, total);
}

void UI_TaskInfo::set_desc(const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_set_desc(*this, s);
}

void UI_TaskInfo::next() {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_next(*this);
}

void UI_TaskInfo::next(const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_next(*this, s);
}

void Engine::tick() {
	ZoneScoped;
}

void Engine::eventloop(int id) {
	ZoneScoped;

	auto last = std::chrono::steady_clock::now();
	double dt = 0;
	bool measured = false;

	while (running.load()) {
		// recompute as logic_gamespeed may change
		double interval_inv = (double)logic_gamespeed * DEFAULT_TICKS_PER_SECOND;
		double interval = 1 / std::max(0.01, interval_inv);

		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - last;
		last = now;
		dt += elapsed.count();

		size_t steps = (size_t)(dt * interval_inv);

		// do steps
		for (; steps; --steps)
			tick();

		dt = fmod(dt, interval);

		unsigned us = 0;

		if (!steps)
			us = (unsigned)(interval * 1000 * 1000);
		// 100000000

		if (us > 500)
			std::this_thread::sleep_for(std::chrono::microseconds(us));
	}
}

void Engine::reserve_threads(int n) {
	if (tp.n_idle() >= n)
		return;

	printf("%s: grow %d\n", __func__, n);
	tp.resize(tp.size() + n);
}

int Engine::mainloop() {
	ZoneScoped;

	running = true;
	reserve_threads(1);
	tp.push([this](int id) { printf("%d\n", !!running.load()); eventloop(id); });

	SDL sdl;

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	const char *glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	const char *glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#pragma warning(disable: 26812)
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#pragma warning(default: 26812)
	SDL_Window *window = SDL_CreateWindow("Age of Empires", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN, window_flags);
	SDL_SetWindowMinimumSize(window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();
	ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	ImVec4 clear_color(0.45f, 0.55f, 0.60f, 1.00f);

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
					if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
						done = true;
					break;
				case SDL_KEYUP:
					p = ImGui_ImplSDL2_ProcessEvent(&event);
					if (!(p && io.WantCaptureKeyboard)) {
						switch (event.key.keysym.sym) {
							case SDLK_BACKQUOTE:
								m_show_menubar = !m_show_menubar;
								break;
						}
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

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		idle();
		display();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
		FrameMark;
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);

	return 0;
}

}
