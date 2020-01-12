#pragma once

#include <memory>

struct SDL_Window;

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_video.h>

class Render {
protected:
	SDL_Window* handle;

	Render(SDL_Window* handle) : handle() {}
public:
	virtual void clear() = 0;
	virtual void paint() = 0;
};

class SimpleRender : public Render {
	std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> handle;
public:
	SimpleRender(SDL_Window* handle, Uint32 flags, int index = -1);

	SDL_Renderer* canvas() { return handle.get(); }

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
	GLRender(SDL_Window* handle, Uint32 flags);
	~GLRender();

	SDL_GLContext context() { return ctx; }

	void clear() override {
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void paint() override {
		SDL_GL_SwapWindow(handle);
	}
};