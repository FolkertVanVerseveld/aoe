#pragma once

#pragma warning(push)
#pragma warning(disable: 26812)
#pragma warning(disable: 26819)

// SDL2 is installed differently per OS
#if _WIN32 || defined(AOE_SDL_NO_PREFIX)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#pragma warning(pop)

namespace aoe {

class SDL final {
public:
	Uint32 flags;
	const char *glsl_version;
	SDL_WindowFlags window_flags;
	SDL_Window *window;
	SDL_GLContext gl_context;

	SDL(Uint32 flags=SDL_INIT_VIDEO | SDL_INIT_TIMER);
	~SDL();
};

}
