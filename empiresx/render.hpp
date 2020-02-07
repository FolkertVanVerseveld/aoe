#pragma once

#include "cfg.hpp"

#include <memory>

struct SDL_Window;

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>

namespace genie {

class Window;

/** Legacy dimensions for drs graphics tied to the 4:3 aspect ratio. */
extern const SDL_Rect lgy_dim[lgy_screen_modes];

/** Enhanced screen dimensions including legacy ones */
extern SDL_Rect scr_dim[screen_modes];

struct Dimensions final {
	SDL_Rect abs_bnds; /**< Absolute dimensions relative to the attached display. */
	SDL_Rect rel_bnds; /**< Relative dimensions within the attached display. */
	SDL_Rect lgy_bnds; /**< Legacy dimensions for graphics tied to the 4:3 aspect ratio. */
	SDL_Rect lgy_orig; /**< Real drawing area starting at (0,0) */

	void resize(const SDL_Rect &abs_bnds);
};

class Render {
protected:
	Window &w;
public:
	Dimensions dim;

	Render(Window &w);
public:
	virtual void chmode(ConfigScreenMode mode) = 0;

	virtual void clear() = 0;
	virtual void paint() = 0;
};

class SimpleRender final : public Render {
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> handle;
public:
	SDL_Rect offset;

	SimpleRender(Window &w, Uint32 flags, int index = -1);

	SDL_Renderer *canvas() { return handle.get(); }

	void legvp();

	void chmode(ConfigScreenMode mode) override;

	void clear() override {
		SDL_RenderClear(handle.get());
	}

	void border(const SDL_Rect &pos, const SDL_Color cols[6], int shade=0);
	void line(int x0, int y0, int x1, int y1);

	void color(SDL_Color c) {
		SDL_SetRenderDrawColor(handle.get(), c.r, c.g, c.b, c.a);
	}

	void paint() override;
};

class GLRender final : public Render {
	SDL_GLContext ctx;
public:
	GLRender(Window &w, Uint32 flags);
	~GLRender();

	SDL_GLContext context() { return ctx; }

	void chmode(ConfigScreenMode mode) override;

	void clear() override;
	void paint() override;
};

class Surface final {
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> handle;
public:
	Surface(const char *fname);
	Surface(SDL_Surface *handle) : handle(handle, &SDL_FreeSurface) {}

	SDL_Surface *data() { return handle.get(); }
	void reset(SDL_Surface *surf) { handle.reset(surf); }
};

class Text;

class Texture final {
	std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> handle;
public:
	int width, height;

	Texture(SimpleRender &r, Surface &s) : handle(NULL, &SDL_DestroyTexture) {
		reset(r, s.data());
	}

	Texture(SimpleRender &r, SDL_Surface *s, bool close = false);
	Texture(int width, int height, SDL_Texture *handle);

	SDL_Texture *data() { return handle.get(); }

	void paint(SimpleRender &r, int x, int y);
	void paint(SimpleRender &r, int x, int y, int w, int h, int sx=0, int sy=0);
	void paint_stretch(SimpleRender &r, const SDL_Rect &to);
	void paint_stretch(SimpleRender &r, const SDL_Rect &from, const SDL_Rect &to);
	void reset(SimpleRender &r, SDL_Surface *surf);
};

}
