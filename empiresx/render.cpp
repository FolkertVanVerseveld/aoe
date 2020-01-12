#include "render.hpp"

#include <stdexcept>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

typedef void* a;
typedef void* a;

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