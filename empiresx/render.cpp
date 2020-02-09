#include "render.hpp"

#include <cassert>

#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

#include "engine.hpp"
#include "menu.hpp"

namespace genie {

const SDL_Rect lgy_dim[lgy_screen_modes] = {
	{0, 0, 640, 480},
	{0, 0, 800, 600},
	{0, 0, 1024, 768},
};

SDL_Rect scr_dim[screen_modes] = {
	{0, 0, 640, 480},
	{0, 0, 800, 600},
	{0, 0, 1024, 768},
	{0, 0, 1024, 768}, // fullscreen mode is based on 1024x768 windowed mode
	{0, 0, 1024, 768}, // custom mode is based on 1024x768 windowed mode
};

int cmp_lgy_dim(int w, int h, int def) {
	for (unsigned i = 0; i < lgy_screen_modes; ++i)
		if (lgy_dim[i].w == w && lgy_dim[i].h == h)
			return i;

	return def;
}

int cmp_dim(int w, int h, int def) {
	for (unsigned i = 0; i < screen_modes; ++i)
		if (scr_dim[i].w == w && scr_dim[i].h == h)
			return i;

	return def;
}

void Dimensions::resize(const SDL_Rect &abs) {
	abs_bnds = abs;

	lgy_orig.x = lgy_orig.y = rel_bnds.x = rel_bnds.y = 0;
	rel_bnds.w = abs_bnds.w;
	rel_bnds.h = abs_bnds.h;

	long long need_w, need_h;

	// project height onto aspect ratio of width
	need_w = abs_bnds.w;
	need_h = need_w * 480 / 640;

	// if too big, project width onto aspect ratio of height
	if (need_h > abs_bnds.h) {
		need_h = abs_bnds.h;
		need_w = need_h * 640 / 480;
	}

	assert(need_w <= abs_bnds.w && need_h <= abs_bnds.h);

	lgy_orig.w = lgy_bnds.w = (int)need_w;
	lgy_orig.h = lgy_bnds.h = (int)need_h;
	lgy_bnds.x = (rel_bnds.w - (int)need_w) / 2;
	lgy_bnds.y = (rel_bnds.h - (int)need_h) / 2;
}

void Render::legvp(bool) {}

Render::Render(Window &w) : w(w), old_dim(), dim(), mode(w.mode()) {
	SDL_Window *win = w.data();

	SDL_Rect bnds;

	SDL_GetWindowPosition(win, &bnds.x, &bnds.y);
	SDL_GetWindowSize(win, &bnds.w, &bnds.h);

	dim.resize(bnds);
}

SimpleRender::SimpleRender(Window &w, Uint32 flags, int index) : Render(w), handle(NULL, &SDL_DestroyRenderer) {
	SDL_Renderer *r;

	if (!(r = SDL_CreateRenderer(w.data(), index, flags)))
		throw std::runtime_error(std::string("Could not initialize SDL renderer: ") + SDL_GetError());

	this->handle.reset(r);

	offset.x = offset.y = 0;
	SDL_GetWindowSize(w.data(), &offset.w, &offset.h);
}

void SimpleRender::legvp(bool legacy) {
	offset = legacy ? dim.lgy_bnds : dim.rel_bnds;
}

void SimpleRender::chmode(ConfigScreenMode old_mode, ConfigScreenMode mode) {
	SDL_Window *win = w.data();
	old_dim = dim;
	SDL_Rect bnds;

	SDL_GetWindowPosition(win, &bnds.x, &bnds.y);
	SDL_GetWindowSize(win, &bnds.w, &bnds.h);

	dim.resize(bnds);
	this->mode = mode;
	nav->resize(old_mode, mode);
}

void SimpleRender::border(const SDL_Rect &pos, const SDL_Color cols[6], int shade, bool flip) {
	int x0 = pos.x, y0 = pos.y, x1 = pos.x + pos.w - 1, y1 = pos.y + pos.h - 1; 

	if (flip) {
		// draw lines from top-left to top-right and top-right to top-bottom
		color(cols[3]);
		line(x0, y0, x1, y0);
		line(x1, y0, x1, y1);

		color(cols[4]);
		line(x0, y0 + 1, x1 - 1, y0 + 1);
		line(x1 - 1, y0 + 1, x1 - 1, y1);

		color(cols[5]);
		line(x0, y0 + 2, x1 - 2, y0 + 2);
		line(x1 - 2, y0 + 2, x1 - 2, y1);

		// draw lines from top-left to bottom-left and bottom-left to bottom-right
		color(cols[0]);
		line(x0 + 2, y0 + 2, x0 + 2, y1 - 2);
		line(x0 + 2, y1 - 2, x1 - 2, y1 - 2);

		color(cols[1]);
		line(x0 + 1, y0 + 1, x0 + 1, y1 - 1);
		line(x0 + 1, y1 - 1, x1 - 1, y1 - 1);

		color(cols[2]);
		line(x0, y0, x0, y1);
		line(x0, y1, x1, y1);
	} else {
		// draw lines from top-left to top-right and top-right to top-bottom
		color(cols[0]);
		line(x0, y0, x1, y0);
		line(x1, y0, x1, y1);

		color(cols[1]);
		line(x0, y0 + 1, x1 - 1, y0 + 1);
		line(x1 - 1, y0 + 1, x1 - 1, y1);

		color(cols[2]);
		line(x0, y0 + 2, x1 - 2, y0 + 2);
		line(x1 - 2, y0 + 2, x1 - 2, y1);

		// draw lines from top-left to bottom-left and bottom-left to bottom-right
		color(cols[3]);
		line(x0 + 2, y0 + 2, x0 + 2, y1 - 2);
		line(x0 + 2, y1 - 2, x1 - 2, y1 - 2);

		color(cols[4]);
		line(x0 + 1, y0 + 1, x0 + 1, y1 - 1);
		line(x0 + 1, y1 - 1, x1 - 1, y1 - 1);

		color(cols[5]);
		line(x0, y0, x0, y1);
		line(x0, y1, x1, y1);
	}

	if (shade) {
		color({0, 0, 0, (uint8_t)shade});
		SDL_Rect box{x0 + offset.x + 2, y0 + offset.y + 2, x1 - x0 - 2, y1 - y0 - 2};

		SDL_BlendMode mode;
		SDL_GetRenderDrawBlendMode(canvas(), &mode);

		SDL_SetRenderDrawBlendMode(canvas(), SDL_BLENDMODE_BLEND);
		SDL_RenderFillRect(canvas(), &box);
		SDL_SetRenderDrawBlendMode(canvas(), mode);
	}
}

void SimpleRender::line(int x0, int y0, int x1, int y1) {
	SDL_RenderDrawLine(canvas(), x0 + offset.x, y0 + offset.y, x1 + offset.x, y1 + offset.y);
}

void SimpleRender::paint() {
	SDL_RenderPresent(handle.get());
	offset = dim.rel_bnds;
}

GLRender::GLRender(Window &w, Uint32 flags) : Render(w) {
	if (!(ctx = SDL_GL_CreateContext(w.data())))
		throw std::runtime_error(std::string("Could not initialize OpenGL renderer: ") + SDL_GetError());

	if (flags & SDL_RENDERER_PRESENTVSYNC)
		SDL_GL_SetSwapInterval(1);
}

GLRender::~GLRender() {
	SDL_GL_DeleteContext(ctx);
}

void GLRender::chmode(ConfigScreenMode old_mode, ConfigScreenMode mode) {
	SDL_Window *win = w.data();
	old_dim = dim;
	SDL_Rect bnds;

	SDL_GetWindowPosition(win, &bnds.x, &bnds.y);
	SDL_GetWindowSize(win, &bnds.w, &bnds.h);
	this->mode = mode;
	dim.resize(bnds);

	assert("stub" == 0);
}

void GLRender::clear() {
	glClear(GL_COLOR_BUFFER_BIT);
}

void GLRender::paint() {
	SDL_GL_SwapWindow(w.data());
}

Surface::Surface(const char *fname) : handle(NULL, &SDL_FreeSurface) {
	SDL_Surface *surf;

	if (!(surf = IMG_Load(fname)))
		throw std::runtime_error(std::string("Could not load image \"") + fname + "\": " + IMG_GetError());

	handle.reset(surf);
}

Texture::Texture(SimpleRender &r, SDL_Surface *s, bool close) : handle(NULL, &SDL_DestroyTexture) {
	reset(r, s);

	if (close)
		SDL_FreeSurface(s);
}

Texture::Texture(int width, int height, SDL_Texture *handle) : handle(handle, &SDL_DestroyTexture), width(width), height(height) {}

void Texture::reset(SimpleRender &r, SDL_Surface *surf) {
	SDL_Texture *tex;

	if (!(tex = SDL_CreateTextureFromSurface(r.canvas(), surf)))
		throw std::runtime_error(std::string("Could not create texture from surface: ") + SDL_GetError());

	width = surf->w;
	height = surf->h;
	handle.reset(tex);
}

void Texture::paint(SimpleRender &r, int x, int y) {
	SDL_Rect dst{x + r.offset.x, y + r.offset.y, width, height};
	SDL_RenderCopy(r.canvas(), data(), NULL, &dst);
}

#ifndef min(a, b)
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

void Texture::paint_stretch(SimpleRender &r, const SDL_Rect &to) {
	SDL_Rect dst(to);
	dst.x = r.offset.x;
	dst.y = r.offset.y;
	SDL_RenderCopy(r.canvas(), data(), NULL, &dst);
}

void Texture::paint_stretch(SimpleRender &r, const SDL_Rect &from, const SDL_Rect &to) {
	SDL_Rect dst(to);
	dst.x = r.offset.x;
	dst.y = r.offset.y;
	SDL_RenderCopy(r.canvas(), data(), &from, &dst);
}

void Texture::paint(SimpleRender &r, int x, int y, int w, int h, int sx, int sy) {
	SDL_Rect src, dest;

	if (!w) w = width;
	if (!h) h = height;

	for (src.y = sy, dest.y = y; dest.y < y + h; src.y = r.offset.y, dest.y += dest.h) {
		src.h = dest.h = min(height, y + h - dest.y);

		for (src.x = sx, dest.x = x; dest.x < x + w; src.x = r.offset.x, dest.x += dest.w) {
			src.w = dest.w = min(width, x + w - dest.x);

			SDL_RenderCopy(r.canvas(), data(), &src, &dest);
		}
	}
}

}
