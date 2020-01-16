#include "render.hpp"

#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>

namespace genie {

SimpleRender::SimpleRender(SDL_Window* handle, Uint32 flags, int index) : Render(handle), handle(NULL, &SDL_DestroyRenderer) {
	SDL_Renderer* r;

	if (!(r = SDL_CreateRenderer(handle, index, flags)))
		throw std::runtime_error(std::string("Could not initialize SDL renderer: ") + SDL_GetError());

	this->handle.reset(r);
}

GLRender::GLRender(SDL_Window* handle, Uint32 flags) : Render(handle) {
	if (!(ctx = SDL_GL_CreateContext(handle)))
		throw std::runtime_error(std::string("Could not initialize OpenGL renderer: ") + SDL_GetError());

	if (flags & SDL_RENDERER_PRESENTVSYNC)
		SDL_GL_SetSwapInterval(1);
}

GLRender::~GLRender() {
	SDL_GL_DeleteContext(ctx);
}

Surface::Surface(const char* fname) : handle(NULL, &SDL_FreeSurface) {
	SDL_Surface* surf;

	if (!(surf = IMG_Load(fname)))
		throw std::runtime_error(std::string("Could not load image \"") + fname + "\": " + IMG_GetError());

	handle.reset(surf);
}

Texture::Texture(SimpleRender& r, SDL_Surface* s, bool close) : handle(NULL, &SDL_DestroyTexture) {
	reset(r, s);
	if (close)
		SDL_FreeSurface(s);
}

Texture::Texture(int width, int height, SDL_Texture* handle) : handle(handle, &SDL_DestroyTexture), width(width), height(height) {}

void Texture::reset(SimpleRender& r, SDL_Surface* surf) {
	SDL_Texture* tex;

	if (!(tex = SDL_CreateTextureFromSurface(r.canvas(), surf)))
		throw std::runtime_error(std::string("Could not create texture from surface: ") + SDL_GetError());

	width = surf->w;
	height = surf->h;
	handle.reset(tex);
}

void Texture::paint(SimpleRender& r, int x, int y) {
	SDL_Rect dst{ x, y, width, height };
	SDL_RenderCopy(r.canvas(), data(), NULL, &dst);
}

}
