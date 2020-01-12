/**
 * Simple quick and dirty AOE demo using my own cmake-sdl2-template
 *
 * See INSTALL for instructions
 *
 * This is just a quick template to get started. Most code is quickly hacked together and could be made more consistent but I won't bother as long as it fucking works
 */

#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <memory>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>

#include "os_macros.hpp"
#include "os.hpp"
#include "net.hpp"
#include "engine.hpp"

#if windows
#pragma comment(lib, "opengl32")
#endif

#define DEFAULT_TITLE "dummy window"

// FIXME move render stuff to separate file

class Render {
protected:
	SDL_Window *handle;

	Render(SDL_Window *handle) : handle(handle) {}
public:
	virtual void clear() = 0;
	virtual void paint() = 0;
};

class SimpleRender : public Render {
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> handle;
public:
	SimpleRender(SDL_Window *handle, Uint32 flags, int index=-1) : Render(handle), handle(NULL, &SDL_DestroyRenderer) {
		SDL_Renderer *r;

		if (!(r = SDL_CreateRenderer(handle, index, flags)))
			throw std::runtime_error(std::string("Could not initialize SDL renderer: ") + SDL_GetError());

		this->handle.reset(r);
	}

	SDL_Renderer *canvas() { return handle.get(); }

	void clear() override {
		SDL_RenderClear(handle.get());
	}

	void color(SDL_Color c) {
		SDL_SetRenderDrawColor(handle.get(), c.r, c.g, c.b, c.a);
	}

	void paint() override {
		SDL_RenderPresent(handle.get());
	}
};

class GLRender : public Render {
	SDL_GLContext ctx;
public:
	GLRender(SDL_Window *handle, Uint32 flags) : Render(handle) {
		if (!(ctx = SDL_GL_CreateContext(handle)))
			throw std::runtime_error(std::string("Could not initialize OpenGL renderer: ") + SDL_GetError());

		if (flags & SDL_RENDERER_PRESENTVSYNC)
			SDL_GL_SetSwapInterval(1);
	}

	~GLRender() {
		SDL_GL_DeleteContext(ctx);
	}

	SDL_GLContext context() { return ctx; }

	void clear() override {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void paint() override {
		SDL_GL_SwapWindow(handle);
	}
};

// FIXME move to engine

namespace genie {

class Window final {
	std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> handle;
	std::unique_ptr<Render> renderer;
	Uint32 flags;
public:
	Window(const char *title, int width, int height, Uint32 flags=SDL_WINDOW_SHOWN) : Window(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags) {}

	Window(const char *title, int x, int y, int width, int height, Uint32 flags=SDL_WINDOW_SHOWN, Uint32 rflags=SDL_RENDERER_PRESENTVSYNC) : handle(NULL, &SDL_DestroyWindow), flags(flags) {
		SDL_Window *w;

		if (!(w = SDL_CreateWindow(title, x, y, width, height, flags)))
			throw std::runtime_error(std::string("Unable to create SDL window: ") + SDL_GetError());

		handle.reset(w);

		if (flags & SDL_WINDOW_OPENGL) {
			renderer.reset(new GLRender(w, rflags));
		} else {
			renderer.reset(new SimpleRender(w, rflags));
		}
	}

	SDL_Window* data() { return handle.get(); }

	Render &render() { return *(renderer.get()); }
};

}

// FIXME move to separate file

class Surface final {
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> handle;
public:
	Surface(const char *fname) : handle(NULL, &SDL_FreeSurface) {
		SDL_Surface *surf;

		if (!(surf = IMG_Load(fname)))
			throw std::runtime_error(std::string("Could not load image \"") + fname + "\": " + IMG_GetError());

		handle.reset(surf);
	}

	Surface(SDL_Surface *handle) : handle(handle, &SDL_FreeSurface) {}

	SDL_Surface *data() { return handle.get(); }
};

class Texture final {
	std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> handle;
public:
	int width, height;

	Texture(SimpleRender &r, Surface &s) : handle(NULL, &SDL_DestroyTexture) {
		reset(r, s.data());
	}

	Texture(SimpleRender& r, SDL_Surface* s, bool close = false) : handle(NULL, &SDL_DestroyTexture) {
		reset(r, s);
		if (close)
			SDL_FreeSurface(s);
	}

	Texture(int width, int height, SDL_Texture *handle) : handle(handle, &SDL_DestroyTexture), width(width), height(height) {}

	SDL_Texture *data() { return handle.get(); }
private:
	void reset(SimpleRender &r, SDL_Surface *surf) {
		SDL_Texture* tex;

		if (!(tex = SDL_CreateTextureFromSurface(r.canvas(), surf)))
			throw std::runtime_error(std::string("Could not create texture from surface: ") + SDL_GetError());

		width = surf->w;
		height = surf->h;
		handle.reset(tex);
	}
};

class Sfx final {
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> handle;
public:
	Sfx(const char *name) : handle(NULL, &Mix_FreeChunk) {
		Mix_Chunk *c;

		if (!(c = Mix_LoadWAV(name)))
			throw std::runtime_error(std::string("Unable to load sound effect \"") + name + "\" : " + Mix_GetError());

		handle.reset(c);
	}

	int play(int loops=0, int channel=-1) {
		return Mix_PlayChannel(channel, handle.get(), loops);
	}

	Mix_Chunk *data() { return handle.get(); }
};

namespace genie {

// FIXME move to font.hpp

class Font final {
	std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> handle;
public:
	const int ptsize;

	Font(const char *fname, int ptsize) : handle(NULL, &TTF_CloseFont), ptsize(ptsize) {
		TTF_Font *f;

		if (!(f = TTF_OpenFont(fname, ptsize)))
			throw std::runtime_error(std::string("Unable to open font \"") + fname + "\": " + TTF_GetError());

		handle.reset(f);
	}

	Font(TTF_Font *handle, int ptsize=12) : handle(handle, &TTF_CloseFont), ptsize(ptsize) {}

	SDL_Surface *surf_solid(const char *text, SDL_Color fg) {
		return TTF_RenderText_Solid(handle.get(), text, fg);
	}

	SDL_Texture *tex_solid(SimpleRender &r, const char *text, SDL_Color fg) {
		std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> s(surf_solid(text, fg), &SDL_FreeSurface);
		return SDL_CreateTextureFromSurface(r.canvas(), s.get());
	}

	TTF_Font *data() { return handle.get(); }
};

}

// FIXME move to font.hpp

class Text final {
	Texture tex;
	std::string str;
public:
	Text(SimpleRender& r, genie::Font& f, const std::string& s, SDL_Color fg) : tex(r, f.surf_solid(s.c_str(), fg)), str(s) {}
};

enum class ConfigScreenMode {
	MODE_640_480,
	MODE_800_600,
	MODE_1024_768,
	MODE_FULLSCREEN,
	MODE_CUSTOM,
};

class Config final {
public:
	/** Startup configuration specified by program arguments. */
	ConfigScreenMode scrmode;
	unsigned poplimit = 50;

	static constexpr unsigned POP_MIN = 25, POP_MAX = 200;

	Config(int argc, char* argv[]);
};

Config::Config(int argc, char* argv[]) : scrmode(ConfigScreenMode::MODE_800_600) {
	unsigned n;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "640")) {
			scrmode = ConfigScreenMode::MODE_640_480;
		}
		else if (!strcmp(argv[i], "800")) {
			scrmode = ConfigScreenMode::MODE_800_600;
		}
		else if (!strcmp(argv[i], "1024")) {
			scrmode = ConfigScreenMode::MODE_1024_768;
		}
		else if (i + 1 < argc && !strcmp(argv[i], "limit")) {
			n = atoi(argv[i + 1]);
			goto set_cap;
		}
		else if (!strcmp(argv[i], "limit=")) {
			n = atoi(argv[i] + strlen("limit="));
		set_cap:
			if (n < POP_MIN)
				n = POP_MIN;
			else if (n > POP_MAX)
				n = POP_MAX;

			poplimit = n;
		}
	}
}

class Menu {
public:
	std::string title;

	Menu(const std::string& t) : title(t) {}

	virtual void paint(SimpleRender &r) {
		r.color(SDL_Color{ 0xff, 0, 0 });
		r.clear();
	}
};

class MenuLobby final : public Menu {
public:
	MenuLobby(SimpleRender& r) : Menu("Multiplayer lobby") {
	}

	void paint(SimpleRender& r) override {
		r.color(SDL_Color{ 0, 0xff, 0xff });
		r.clear();
	}
};

int main(int argc, char **argv)
{
	try {
		OS os;
		Config cfg(argc, argv);

		std::cout << "hello " << os.username << " on " << os.compname << '!' << std::endl;

		Engine eng;

#if windows
		char path_fnt[] = "D:\\SYSTEM\\FONTS\\ARIAL.TTF";
		TTF_Font *h_fnt = NULL;

		// find cdrom drive
		for (char dl = 'D'; dl <= 'Z'; ++dl) {
			path_fnt[0] = dl;

			if ((h_fnt = TTF_OpenFont(path_fnt, 13)) != NULL)
				break;
		}
#else
		char path_fnt[] = "/media/cdrom/SYSTEM/FONTS/ARIAL.TTF";
		TTF_Font *h_fnt = TTF_OpenFont(path_fnt, 13);
#endif

		if (!h_fnt) {
			std::cerr << "default font not found" << std::endl;
			return 1;
		}

		// figure out window dimensions
		int width, height;

		switch (cfg.scrmode) {
		case ConfigScreenMode::MODE_640_480: width = 640; height = 480; break;
		case ConfigScreenMode::MODE_1024_768: width = 1024; height = 768; break;
		case ConfigScreenMode::MODE_800_600:
		default:
			width = 800; height = 600; break;
		}

		// run simple demo with SDL_Renderer
		genie::Window w(DEFAULT_TITLE, width, height);

#if windows
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		if (!SDL_GetWindowWMInfo(w.data(), &info))
			throw std::runtime_error(std::string("Could not get window WM info: ") + SDL_GetError());
#endif

		SimpleRender& r = (SimpleRender&)w.render();

		genie::Font fnt(h_fnt);
		Surface host_surf(fnt.surf_solid("host?", SDL_Color{ 0, 0xff, 0 }));
		Texture host_tex(r, host_surf);

		Surface test("joi_rebels.jpg");
		Texture tex(r, test);

		MenuLobby menu(r);

#if windows
		HFONT hfont;
		RECT rect;
		TEXTMETRIC fm;
		HDC hdc = info.info.win.hdc;
		int pt = 28;

		hfont = CreateFont(-pt, 0, 0, 0,
						   FW_DONTCARE,
						   FALSE, 0, FALSE,
						   DEFAULT_CHARSET, 0, CLIP_DEFAULT_PRECIS,
						   CLEARTYPE_QUALITY,
						   VARIABLE_PITCH,
						   path_fnt);
		if (!hfont)
			throw std::runtime_error(std::string("Could not create HFONT: code ") + std::to_string(GetLastError()));
#endif

		for (bool running = true; running;) {
			SDL_Event ev;

			while (SDL_PollEvent(&ev)) {
				switch (ev.type) {
				case SDL_QUIT:
					running = false;
					continue;
				}
			}

			menu.paint(r);

			SDL_RenderCopy(r.canvas(), tex.data(), NULL, NULL);
			SDL_Rect pos{ 0, 0, host_tex.width, host_tex.height };
			SDL_RenderCopy(r.canvas(), host_tex.data(), NULL, &pos);

#if windows
			HGDIOBJ old = SelectObject(hdc, hfont);
			{
				//SetBkColor(hdc, RGB(0xa0, 0x00, 0x00));
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, RGB(0xff, 0xff, 0xff));

				TextOut(hdc, 40, 40, "do you want to host?", strlen("do you want to host?"));
			}
			SelectObject(hdc, old);
#endif

			r.paint();
		}

#if windows
		DeleteObject(hfont);
#endif
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
