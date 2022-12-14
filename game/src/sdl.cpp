#include "sdl.hpp"

#include <stdexcept>
#include <string>

#include <tracy/Tracy.hpp>

#include "legacy.hpp"

namespace aoe {

SDLguard::SDLguard(Uint32 flags) : flags(flags)
#if defined(IMGUI_IMPL_OPENGL_ES2)
	, glsl_version("#version 100")
#elif defined(__APPLE__)
	, glsl_version("#version 150")
#else
	, glsl_version("#version 130")
#endif
{
	ZoneScoped;
	if (SDL_Init(flags))
		throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
}

SDLguard::~SDLguard() {
	ZoneScoped;

	SDL_Quit();
}

SDL::SDL(Uint32 flags) : guard(flags)
#pragma warning(disable: 26812)
, window_flags((SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI))
#pragma warning(default: 26812)
, window(nullptr), gl_context(nullptr)
{
	window = SDL_CreateWindow("Age of Empires", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN, window_flags);
	SDL_SetWindowMinimumSize(window, WINDOW_WIDTH_MIN, WINDOW_HEIGHT_MIN);
	gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync
}

SDL::~SDL() {
	if (gl_context) SDL_GL_DeleteContext(gl_context);
	if (window) SDL_DestroyWindow(window);
}

}
