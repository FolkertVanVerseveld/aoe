#include "sdl.hpp"

#include <stdexcept>
#include <string>

namespace aoe {

SDL::SDL(Uint32 flags) : flags(flags) {
	if (SDL_Init(flags))
		throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
}

SDL::~SDL() {
	SDL_Quit();
}

}
