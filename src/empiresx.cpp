 // dear imgui: standalone example application for SDL2 + OpenGL
 // If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
 // (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
 // (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"

// addons
#include "imgui/ImGuiFileDialog.h"

#include "prodcons.hpp"
#include "lang.hpp"
#include "drs.hpp"
#include "math.hpp"

#include <cstdio>
#include <ctime>

#include <atomic>
#include <string>
#include <thread>
#include <variant>
#include <utility>
#include <memory>
#include <future>
#include <algorithm>
#include <stack>
#include <exception>
#include <stdexcept>

#include <signal.h>

#include <SDL2/SDL.h>

#define FDLG_CHOOSE_DIR "Fdlg Choose AoE dir"
#define MSG_INIT "Msg Init"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

static const SDL_Rect dim_lgy[] = {
	{0, 0, 640, 480},
	{0, 0, 800, 600},
	{0, 0, 1024, 768},
};

#pragma warning(disable : 26812)

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#if __APPLE__
// GL 3.2 Core + GLSL 150
const char *glsl_version = "#version 150";
#else
// GL 3.0 + GLSL 130
const char *glsl_version = "#version 130";
#endif

class Shader final {
public:
	const GLchar *src;
	GLuint handle;
	GLenum type;

	Shader(const GLchar *src, GLenum type) : src(src), handle(0), type(type) {}

	void compile() {
		const GLchar *srclist[] = {src};
		handle = glCreateShader(type);
		if (!handle) {
			fputs("fatal error: glCreateShader\n", stderr);
			abort();
		}
		glShaderSource(handle, 1, srclist, NULL);
		glCompileShader(handle);

		GLint status, length;
		glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);

		if (length > 0) {
			std::string msg(length, 0);
			glGetShaderInfoLog(handle, length, NULL, (GLchar*)msg.data());
			fprintf((GLboolean)status == GL_FALSE ? stderr : stdout, "%s\n", msg.c_str());
		}

		if ((GLboolean)status == GL_FALSE) {
			fprintf(stderr, "fatal error: glCompileShader type %u\n", type);
			abort();
		}
	}
} shader_vtx(R"glsl(
#version 120
attribute vec2 coord2d;
void main(void)
{
	gl_Position = vec4(coord2d, 0.0, 1.0);
}
)glsl", GL_VERTEX_SHADER)
, shader_frag(R"glsl(
#version 120
void main(void) {
	gl_FragColor[0] = 0.0;
	gl_FragColor[1] = 0.0;
	gl_FragColor[2] = 1.0;
}
)glsl", GL_FRAGMENT_SHADER);

static GLint attr_pos;

class ShaderProgram final {
public:
	GLuint vtx, frag;
	GLuint handle;

	ShaderProgram(GLuint vtx, GLuint frag) : vtx(vtx), frag(frag), handle(0) {
		if (!(handle = glCreateProgram())) {
			fputs("fatal error: glCreateProgram\n", stderr);
			abort();
		}

		glAttachShader(handle, vtx);
		glAttachShader(handle, frag);
		glLinkProgram(handle);

		GLint status, length;
		glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
		glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);

		if (length > 0) {
			std::string msg(length, 0);
			glGetShaderInfoLog(handle, length, NULL, (GLchar*)msg.data());
			fprintf((GLboolean)status == GL_FALSE ? stderr : stdout, "%s\n", msg.c_str());
		}

		if ((GLboolean)status == GL_FALSE) {
			fputs("fatal error: glLinkProgram\n", stderr);
			abort();
		}
	}
};

static bool show_debug;
static bool fs_has_root;
static std::atomic_bool running(true);

enum class MenuId {
	config,
	start,
	editor,
};

MenuId menu_id = MenuId::config;

enum class WorkTaskType {
	stop,
	check_root,
};

using FdlgItem = std::pair<std::string, std::string>;

class WorkTask final {
public:
	WorkTaskType type;
	std::variant<std::nullopt_t, FdlgItem> data;

	WorkTask(WorkTaskType t) : type(t), data(std::nullopt) {}

	template<typename... Args>
	WorkTask(WorkTaskType t, Args&&... args) : type(t), data(args...) {}
};

enum class WorkResultType {
	stop,
	success,
	fail,
};

class WorkResult final {
public:
	WorkTaskType task_type;
	WorkResultType type;
	std::string what;

	WorkResult(WorkTaskType tt, WorkResultType rt) : task_type(tt), type(rt), what() {}
	WorkResult(WorkTaskType tt, WorkResultType rt, const std::string &what) : task_type(tt), type(rt), what(what) {}
};

struct WorkerProgress final {
	std::atomic<unsigned> step;
	unsigned total;
	std::string desc;
};

// trick to abort worker thread on shutdown
struct WorkInterrupt final {
	WorkInterrupt() {}
};

namespace genie {

class Assets final {
public:
	std::vector<std::unique_ptr<DRS>> blobs;
} assets;

}

class Worker final {
public:
	ConcurrentChannel<WorkTask> tasks; // in
	ConcurrentChannel<WorkResult> results; // out

	std::mutex mut;
	WorkerProgress p;

	Worker() : tasks(WorkTaskType::stop, 10), results(WorkResult{ WorkTaskType::stop, WorkResultType::stop }, 10), mut(), p() {}

	int run() {
		try {
			while (tasks.is_open()) {
				WorkTask task = tasks.consume();

				switch (task.type) {
					case WorkTaskType::stop:
						continue;
					case WorkTaskType::check_root:
						check_root(std::get<std::pair<std::string, std::string>>(task.data));
						break;
					default:
						assert("bad task type" == 0);
						break;
				}
			}
		} catch (WorkInterrupt&) {
			done(WorkTaskType::stop, WorkResultType::stop);
			return 1;
		}

		return 0;
	}

	bool progress(WorkerProgress &p) {
		std::unique_lock<std::mutex> lock(mut);

		unsigned v = this->p.step.load();

		p.desc = this->p.desc;
		p.step.store(v);
		p.total = this->p.total;

		return v != p.total;
	}

private:
	void start(unsigned total, unsigned step=0) {
		std::unique_lock<std::mutex> lock(mut);
		if (!running)
			throw WorkInterrupt();
		p.step = step;
		p.total = total;
	}

	void set_desc(const std::string &s) {
		std::unique_lock<std::mutex> lock(mut);
		if (!running)
			throw WorkInterrupt();
		p.desc = s;
	}

	template<typename... Args>
	void done(Args&&... args) {
		std::unique_lock<std::mutex> lock(mut);
		p.step = p.total;
		results.produce(args...);
	}

	void check_root(const std::pair<std::string, std::string> &item) {
#define COUNT 5

		genie::iofd fd[COUNT];
		// windows doesn't care about case sensitivity, but we need to make sure this also works on *n*x
		std::string fnames[COUNT] = {
			"Border.drs",
			"graphics.drs",
			"Interfac.drs",
			"sounds.drs",
			"Terrain.drs"
		};

		struct ResChk final {
			genie::res_id id;
			unsigned type;
		};

		// just list some important assests to take an educated guess if everything looks fine
		const std::vector<std::vector<ResChk>> res = {
			{ // border
				{20000, 2}, {20001, 2}, {20002, 2}, {20003, 2}, {20004, 2}, {20006, 2},
			},{ // graphics
				{230, 2}, {254, 2}, {280, 2}, {418, 2}, {425, 2}, {463, 2},
			},{ // interface
				{50057, 0}, {50058, 0}, {50060, 0}, {50061, 0}, {50721, 2}, {50731, 2}, {50300, 3}, {50302, 3}, {50303, 3}, {50320, 3}, {50321, 3},
			},{ // sounds
				{5036, 3}, {5092, 3}, {5107, 3},
			},{ // terrain
				{15000, 2}, {15001, 2}, {15002, 2}, {15003, 2},
			}
		};

		start(2);
		set_desc(TXT(TextId::work_drs));

		try {
			for (unsigned i = 0; i < COUNT; ++i) {
				std::string path(genie::drs_path(item.first, fnames[i]));
				if ((fd[i] = genie::open(path.c_str())) == genie::fd_invalid) {
					for (unsigned j = 0; j < i; ++j)
						genie::close(fd[j]);

					throw std::runtime_error(path);
				}
			}
		} catch (std::runtime_error &e) {
			if (item.first == item.second) {
				done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_path, e.what()));
				return;
			}
			try {
				for (unsigned i = 0; i < COUNT; ++i) {
					std::string path(genie::drs_path(item.second, fnames[i]));
					if ((fd[i] = genie::open(path.c_str())) == genie::fd_invalid) {
						for (unsigned j = 0; j < i; ++j)
							genie::close(fd[j]);

						throw std::runtime_error(path);
					}
				}
			} catch (std::runtime_error &e) {
				done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_path, e.what()));
				return;
			}
		}

		genie::assets.blobs.clear();

		for (unsigned i = 0; i < COUNT; ++i) {
			try {
				genie::assets.blobs.emplace_back(new genie::DRS(fnames[i], fd[i], i != 3));
			} catch (std::runtime_error &e) {
				for (unsigned j = i + 1; j < COUNT; ++j)
					genie::close(fd[j]);

				genie::assets.blobs.clear();
				done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_path, e.what()));
				return;
			}
		}

		++p.step;
		set_desc("Verifying data resource sets");

		for (unsigned i = 0; i < res.size(); ++i) {
			genie::io::DrsItem dummy;

			const std::vector<ResChk> &l = res[i];
			for (auto &r : l)
				if (!genie::assets.blobs[i]->open_item(dummy, r.id, (genie::DrsType)r.type)) {
					done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_no_drs_item, (unsigned)r.id));
					return;
				}
		}

		++p.step;
		done(WorkTaskType::check_root, WorkResultType::success, TXT(TextId::work_drs_success));
#undef COUNT
	}
};

static void worker_init(Worker &w, int &status) {
	status = w.run();
}

// Main code
int main(int, char**)
{
	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	// Decide GL+GLSL versions
#if __APPLE__
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Window *window = SDL_CreateWindow("Age of Empires Remake", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// resizing is OK, but not too small please
	SDL_SetWindowMinimumSize(window, dim_lgy[0].w, dim_lgy[0].h);

	// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
	bool err = false;
	glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
	bool err = false;
	glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
	bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
	if (err)
	{
		fprintf(stderr, "Failed to initialize OpenGL loader!\n");
		return 1;
	}

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	time_t t_start = time(NULL);
	struct tm *tm_start = localtime(&t_start);
	int year_start = std::max(tm_start->tm_year, 2020);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());

	// Our state
	bool show_demo_window = true;
	bool ther_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	auto fd = igfd::ImGuiFileDialog::Instance();

	bool fs_choose_root = false, auto_detect = false;
	Worker w_bkg;
	int err_bkg;

	std::thread t_bkg(worker_init, std::ref(w_bkg), std::ref(err_bkg));
	WorkerProgress p = { 0 };
	std::string bkg_result;

	std::stack<const char*> auto_paths;
#if windows
	auto_paths.emplace("C:\\Program Files (x86)\\Microsoft Games\\Age of Empires");
	auto_paths.emplace("C:\\Program Files\\Microsoft Games\\Age of Empires");
#else
	auto_paths.emplace("/media/cdrom");
#endif

	if (!auto_paths.empty()) {
		auto_detect = true;
		std::string path(auto_paths.top());
		w_bkg.tasks.produce(WorkTaskType::check_root, std::make_pair(path, path));
		auto_paths.pop();
	}

	// while our worker thread is auto detecting, we can setup the shaders
	shader_vtx.compile();
	shader_frag.compile();

	ShaderProgram prog(shader_vtx.handle, shader_frag.handle);
	attr_pos = glGetAttribLocation(prog.handle, "coord2d");

	if (attr_pos == -1) {
		fputs("fatal error: glGetAttribLocation\n", stderr);
		abort();
	}

	glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);

	// Main loop
	while (running)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			bool munched, p;

			// check if imgui munches event
			switch (event.type) {
			case SDL_MOUSEWHEEL:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				p = ImGui_ImplSDL2_ProcessEvent(&event);
				munched = p && io.WantCaptureMouse;
				break;
			case SDL_TEXTINPUT:
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				p = ImGui_ImplSDL2_ProcessEvent(&event);
				munched = p && io.WantCaptureKeyboard;
				break;
			default:
				munched = ImGui_ImplSDL2_ProcessEvent(&event);
				break;
			}

			if (event.type == SDL_QUIT)
				running = false;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				running = false;

			if (!munched) {
				switch (event.type) {
				case SDL_KEYUP:
					if (event.key.keysym.sym == SDLK_BACKQUOTE)
						show_debug = !show_debug;
					break;
				}
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

		if (show_debug) {
			// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
			if (show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);

			ImGui::Begin("Debug control");
			{
				ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

				static int current_fault = 0;
				static const char *fault_types[] = {"Segmentation violation", "NULL dereference", "_Exit", "exit", "abort", "std::terminate", "throw int"};

				ImGui::Combo("Fault type", &current_fault, fault_types, IM_ARRAYSIZE(fault_types));

				if (ImGui::Button("Oops")) {
					switch (current_fault) {
						default: raise(SIGSEGV); break;
						// NULL dereferencing directly is usually optimised out in modern compilers, so use a trick to bypass this...
						case 1: { int *p = (int*)1; -1[p]++; } break;
						case 2: _Exit(1); break;
						case 3: exit(1); break;
						case 4: abort(); break;
						case 5: std::terminate(); break;
						case 6: throw 42; break;
					}
				}
			}
			ImGui::End();
		}

		static int lang_current = 0;
		static bool show_about = false;
		bool working = w_bkg.progress(p);

		// munch all results
		for (std::optional<WorkResult> res; res = w_bkg.results.try_consume(), res.has_value();) {
			switch (res->type) {
				case WorkResultType::success:
					switch (res->task_type) {
						case WorkTaskType::stop:
							break;
						case WorkTaskType::check_root:
							fs_has_root = true;
							// start game automatically on auto detect
							if (auto_detect) {
								auto_detect = false;
								menu_id = MenuId::start;
							}
							break;
						default:
							assert("bad task type" == 0);
							break;
					}
					break;
				case WorkResultType::stop:
					break;
				case WorkResultType::fail:
					if (!auto_paths.empty()) {
						std::string path(auto_paths.top());
						w_bkg.tasks.produce(WorkTaskType::check_root, std::make_pair(path, path));
						auto_paths.pop();
						working = true;
					} else {
						auto_detect = false;
					}
					break;
				default:
					assert("bad result type" == 0);
					break;
			}
			bkg_result = res->what;
		}

		switch (menu_id) {
			case MenuId::config:
				ImGui::Begin("Startup configuration", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
				{
					ImGui::SetWindowSize(io.DisplaySize);
					ImGui::SetWindowPos(ImVec2());

					if (ImGui::Button(TXT(TextId::btn_about)))
						show_about = true;

					ImGui::TextWrapped(TXT(TextId::hello));

					lang_current = int(lang);
					ImGui::Combo(TXT(TextId::language), &lang_current, langs, int(LangId::max));
					lang = (LangId)genie::clamp(0, lang_current, int(LangId::max) - 1);

					if (!working && ImGui::Button(TXT(TextId::set_game_dir)))
						fd->OpenDialog(FDLG_CHOOSE_DIR, TXT(TextId::dlg_game_dir), 0, ".");

					if (fd->FileDialog(FDLG_CHOOSE_DIR, ImGuiWindowFlags_NoCollapse, ImVec2(400, 200)) && !working) {
						if (fd->IsOk == true) {
							std::string fname(fd->GetFilepathName()), path(fd->GetCurrentPath());
							printf("fname = %s\npath = %s\n", fname.c_str(), path.c_str());

							fs_has_root = false;
							w_bkg.tasks.produce(WorkTaskType::check_root, std::make_pair(fname, path));
						}

						fd->CloseDialog(FDLG_CHOOSE_DIR);
					}

					if (working && p.total) {
						ImGui::TextUnformatted(p.desc.c_str());
						ImGui::ProgressBar(float(p.step) / p.total);
					}

					if (!bkg_result.empty())
						ImGui::TextUnformatted(bkg_result.c_str());

					if (ImGui::Button(TXT(TextId::btn_quit)))
						running = false;

					if (fs_has_root) {
						ImGui::SameLine();
						if (ImGui::Button(TXT(TextId::btn_startup)))
							menu_id = MenuId::start;
					}
				}
				ImGui::End();
				break;
			case MenuId::start:
				ImGui::Begin("Main menu placeholder");
				{
					if (ImGui::Button(TXT(TextId::btn_about)))
						show_about = true;

					if (ImGui::Button(TXT(TextId::btn_editor)))
						menu_id = MenuId::editor;

					if (ImGui::Button(TXT(TextId::btn_quit)))
						running = false;

					lang_current = int(lang);
					ImGui::Combo(TXT(TextId::language), &lang_current, langs, int(LangId::max));
					lang = (LangId)genie::clamp(0, lang_current, int(LangId::max) - 1);
				}
				ImGui::End();
				break;
			case MenuId::editor:
				ImGui::Begin("Editor menu placeholder");
				{
					if (ImGui::Button(TXT(TextId::btn_back)))
						menu_id = MenuId::start;
				}
				ImGui::End();
				break;
		}

		if (show_about) {
			ImGui::Begin("About", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
			ImGui::SetWindowSize(io.DisplaySize);
			ImGui::SetWindowPos(ImVec2());
			ImGui::TextWrapped("Copyright 2016-%d Folkert van Verseveld\n", year_start);
			ImGui::TextWrapped(TXT(TextId::about));
			ImGui::TextWrapped("Using OpenGL version: %d.%d", major, minor);

			if (ImGui::Button(TXT(TextId::btn_back)))
				show_about = false;
			ImGui::End();
		}

		// Rendering
		ImGui::Render();

		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT);

		// do our stuff
		switch (menu_id) {
			case MenuId::config:
				break;
			case MenuId::start:
			{
				glUseProgram(prog.handle);
				glEnableVertexAttribArray(attr_pos);
				const GLfloat vtx[] = {
					 0.0,  0.8,
					-0.8, -0.8,
					 0.8, -0.8,
				};
				glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, vtx);
				glDrawArrays(GL_TRIANGLES, 0, 3);
				glDisableVertexAttribArray(attr_pos);
				break;
			}
			case MenuId::editor:
				break;
		}

		// do imgui stuff
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(window);
	}

	// try to stop worker
	// for whatever reason, it may be busy for a long time (e.g. blocking socket call)
	// in that case, wait up to 3 seconds
	w_bkg.tasks.stop();
	auto future = std::async(std::launch::async, &std::thread::join, &t_bkg);
	if (future.wait_for(std::chrono::seconds(3)) == std::future_status::timeout)
		_Exit(0); // worker still busy, dirty exit

	// disable saving imgui.ini
	io.IniFilename = NULL;

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
