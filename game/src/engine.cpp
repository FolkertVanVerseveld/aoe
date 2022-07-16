#include "engine.hpp"

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include <SDL2/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL2/SDL_opengles2.h>
#else
#include <SDL2/SDL_opengl.h>
#endif

#include "imgui_user.hpp"

#include <cstdio>
#include <cstdint>

#include <mutex>
#include <memory>

#include <fstream>
#include <stdexcept>
#include <string>

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
	std::ofstream out(path, std::ios_base::binary | std::ios_base::trunc);

	// let c++ take care of any errors
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	uint32_t magic = 0x06ce09f6;

	out.write((const char*)&magic, sizeof magic);
	write(out, bnds);
	write(out, display);
	write(out, vp);
}

Engine::Engine()
	: net(), show_demo(false)
	, connection_mode(0), connection_port(32768), connection_host("")
	, menu_state(MenuState::init), multiplayer_ready(false), cfg("config"), scn()
	, chat_line()
{
	std::lock_guard<std::mutex> lk(m_eng);
	if (eng)
		throw std::runtime_error("there can be only one");

	eng = this;
}

Engine::~Engine() {
	std::lock_guard<std::mutex> lk(m_eng);
	assert(eng);
	eng = nullptr;
}

void Engine::show_menubar() {
	if (!ImGui::BeginMainMenuBar())
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
			strcpy(connection_host, "127.0.0.1");
	}

	ImGui::InputScalar("port", ImGuiDataType_U16, &connection_port);

	if (ImGui::Button("start")) {
		switch (connection_mode) {
			case 0: menu_state = MenuState::multiplayer_host; break;
			case 1: break;
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("quit"))
		throw 0;

	ImGui::End();
}

void Engine::show_multiplayer_host() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	if (!ImGui::Begin("multiplayer host", NULL, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse)) {
		ImGui::End();
		return;
	}

	ImGui::SetWindowSize(vp->WorkSize);

	ImGui::TextUnformatted("Multiplayer game");

	float frame_height = 0.85f;
	float player_height = 0.7f;
	float frame_margin = 0.075f;

	ImGui::BeginChild("LeftFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.7f, ImGui::GetWindowHeight() * frame_height));
	ImGui::BeginChild("PlayerFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * player_height), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::BeginTable("PlayerTable", 4);

	ImGui::PushID(-1);
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
	ImGui::TextUnformatted("Name");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Civ");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Player");
	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Team");
	ImGui::PopID();

	for (unsigned i = 0; i < 8; ++i) {
		ImGui::PushID(i);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("player %u", i + 1);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Macedonian");
		ImGui::TableNextColumn();
		ImGui::Text("%u", i + 1);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("-");
		ImGui::PopID();
	}
	ImGui::EndTable();
	ImGui::EndChild();
	ImGui::BeginChild("ChatFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * (1 - player_height - frame_margin)), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::TextUnformatted("Chat");
	// TODO add chat
	ImGui::InputText("##", chat_line);
	ImGui::EndChild();
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::BeginChild("SettingsFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.3f, ImGui::GetWindowHeight() * frame_height), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::Button("Settings")) {
	}

	ImGui::TextUnformatted("Scenario: Random map");

	ImGui::InputScalar("Map width", ImGuiDataType_U32, &scn.width);
	ImGui::InputScalar("Map height", ImGuiDataType_U32, &scn.height);

	ImGui::Checkbox("Fixed position", &scn.fixed_start);
	ImGui::Checkbox("Reveal map", &scn.explored);
	ImGui::Checkbox("Full Tech Tree", &scn.all_technologies);
	ImGui::Checkbox("Enable cheating", &scn.cheating);

	ImGui::InputScalar("Population limit", ImGuiDataType_U32, &scn.popcap);

	ImGui::EndChild();

	ImGui::Checkbox("I'm Ready!", &multiplayer_ready);

	ImGui::SameLine();

	if (ImGui::Button("Start Game"))
		menu_state = MenuState::multiplayer_game;

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
		menu_state = MenuState::init;

	ImGui::End();
}

void Engine::display() {
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
}

int Engine::mainloop() {
	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

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
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window = SDL_CreateWindow("Age of Empires", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, window_flags);
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
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		display();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

}
