#pragma once

// SDL2 is installed differently per OS
#if _WIN32 || defined(AOE_SDL_NO_PREFIX)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
