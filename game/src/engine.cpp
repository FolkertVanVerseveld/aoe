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
	, chat_line(), chat(), m(), m_async(), m_ui(), server()
	, tp(2), ui_tasks(), ui_mod_id(), popups()
	, tsk_start_server{ invalid_ref }, async_tasks(0)
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
#if 0
				try {
					start_server(connection_port);
					scn.hosting = true;
					next_menu_state = MenuState::multiplayer_host;
				} catch (std::exception &e) {
					fprintf(stderr, "%s: cannot start server: %s\n", __func__, e.what());
					push_error(std::string("cannot start server: ") + e.what());
				}
#else
				start_server2(connection_port);
#endif
				break;
			case 1:
				try {
					start_client(connection_host, connection_port);
					scn.hosting = false;
					next_menu_state = MenuState::multiplayer_host;
				} catch (std::exception &e) {
					fprintf(stderr, "%s: cannot join server: %s\n", __func__, e.what());
					push_error(std::string("cannot join server: ") + e.what());
				}
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
	popups.emplace(msg, ui::PopupType::error);
}

void Engine::start_client(const char *host, uint16_t port) {
	ZoneScoped;
	client.reset(new Client());
	client->start(host, port);
}

void Engine::start_server(uint16_t port) {
	ZoneScoped;
	tp.push([this](int id, uint16_t port) {
		ZoneScoped;
		try {
			{
				std::lock_guard<std::mutex> lk(m);
				server.reset(new Server);
			}
			cv_server_start.notify_all();
			// XXX this is racy and i don't like it... but we just cannot keep the lock for the complete function call as there will be no way to cancel it without deadlocking...
			server->mainloop(id, port, 1);
		} catch (std::exception &e) {
			fprintf(stderr, "%s: cannot start server: %s\n", __func__, e.what());
			push_error(std::string("cannot start server: ") + e.what());
		}
	}, port);

	// wait for server to start
	using namespace std::chrono_literals;

	std::unique_lock<std::mutex> lk(m);
	if (cv_server_start.wait_for(lk, 500ms, [&] {return server.get() != nullptr; })) {
		start_client("127.0.0.1", port);
	} else {
		// error time out
		fprintf(stderr, "%s: cannot start server: internal server error\n", __func__);
		push_error("cannot start server: internal server error");
		server->stop();
	}
}

void Engine::stop_server_now() {
	ZoneScoped;

	std::lock_guard<std::mutex> lk(m);
	if (server) {
		server->close();
		tsk_start_server = invalid_ref;
	}
}

void Engine::stop_server() {
	ZoneScoped;

	tp.push([this](int id) {
		std::lock_guard<std::mutex> lk(m_eng);
		if (eng)
			stop_server_now();
	});
}

void Engine::start_server2(uint16_t port) {
	ZoneScoped;

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
					if (eng) {
						assert(eng->tsk_start_server == invalid_ref);
						eng->tsk_start_server = info.get_ref();
					}
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

			server.reset(new Server);

			tp.push([this](int id, uint16_t port) {
				server->mainloop(id, port, 1);
			}, port);

			info.next("Connecting to host");

			start_client("127.0.0.1", port);

			guard.good = true;
		} catch (std::exception &e) {
			fprintf(stderr, "%s: cannot start server: %s\n", __func__, e.what());
		}
	}, port);
}

void Engine::trigger_server_started() {
	std::lock_guard<std::mutex> lock(m_async);
	async_tasks |= (unsigned)EngineAsyncTask::server_started;
}

void Engine::idle() {
	ZoneScoped;
	idle_async();
	menu_state = next_menu_state;
}

void Engine::idle_async() {
	ZoneScoped;

	std::lock_guard<std::mutex> lock(m_async);

	if (async_tasks) {
		if (async_tasks & (unsigned)EngineAsyncTask::server_started)
			next_menu_state = MenuState::multiplayer_host;
	}

	async_tasks = 0;
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

int Engine::mainloop() {
	ZoneScoped;
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
