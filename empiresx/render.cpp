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

Render::Render(Window &w) : w(w), abs_bnds(), rel_bnds() {
	SDL_Window *win = w.data();

	SDL_GetWindowPosition(win, &abs_bnds.x, &abs_bnds.y);
	SDL_GetWindowSize(win, &abs_bnds.w, &abs_bnds.h);

	rel_bnds.x = rel_bnds.y = 0;
	rel_bnds.w = abs_bnds.w;
	rel_bnds.h = abs_bnds.h;
}

SimpleRender::SimpleRender(Window &w, Uint32 flags, int index) : Render(w), handle(NULL, &SDL_DestroyRenderer) {
	SDL_Renderer *r;

	if (!(r = SDL_CreateRenderer(w.data(), index, flags)))
		throw std::runtime_error(std::string("Could not initialize SDL renderer: ") + SDL_GetError());

	this->handle.reset(r);
}

void SimpleRender::chmode(ConfigScreenMode mode) {
	(void)mode;

	SDL_Window *win = w.data();
	SDL_Rect old_abs = abs_bnds, old_rel = rel_bnds;

	SDL_GetWindowPosition(win, &abs_bnds.x, &abs_bnds.y);
	SDL_GetWindowSize(win, &abs_bnds.w, &abs_bnds.h);

	// TODO keep aspect ratio
	rel_bnds.x = rel_bnds.y = 0;
	rel_bnds.w = abs_bnds.w;
	rel_bnds.h = abs_bnds.h;

	nav->resize(old_abs, abs_bnds, old_rel, rel_bnds);
}

void SimpleRender::border(const SDL_Rect &pos, const SDL_Color cols[6]) {
	int x0 = pos.x, y0 = pos.y, x1 = pos.x + pos.w - 1, y1 = pos.y + pos.h - 1; 

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

void SimpleRender::line(int x0, int y0, int x1, int y1) {
	SDL_RenderDrawLine(canvas(), x0, y0, x1, y1);
}

void SimpleRender::paint() {
	SDL_RenderPresent(handle.get());
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

void GLRender::chmode(ConfigScreenMode mode) {
	assert("stub" == 0);
}

void GLRender::clear() {
	glClear(GL_COLOR_BUFFER_BIT);
}

void GLRender::paint() {
	SDL_GL_SwapWindow(w.data());
}

Surface::Surface(const char *fname) : handle(NULL, &SDL_FreeSurface) {
	SDL_Surface* surf;

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
	SDL_Rect dst{x, y, width, height};
	SDL_RenderCopy(r.canvas(), data(), NULL, &dst);
}

#ifndef min(a, b)
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

void Texture::paint(SimpleRender &r, int x, int y, int w, int h, int sx, int sy) {
	SDL_Rect src, dest;

	for (src.y = sy, dest.y = y; dest.y < y + h; src.y = 0, dest.y += dest.h) {
		src.h = dest.h = min(height, y + h - dest.y);

		for (src.x = sx, dest.x = x; dest.x < x + w; src.x = 0, dest.x += dest.w) {
			src.w = dest.w = min(width, x + w - dest.x);

			SDL_RenderCopy(r.canvas(), data(), &src, &dest);
		}
	}
}

}
