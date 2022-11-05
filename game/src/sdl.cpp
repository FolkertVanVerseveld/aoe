#include "sdl.hpp"

#include <stdexcept>
#include <string>

#include <tracy/Tracy.hpp>

namespace aoe {

SDL::SDL(Uint32 flags) : flags(flags) {
	ZoneScoped;
	if (SDL_Init(flags))
		throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
}

SDL::~SDL() {
	ZoneScoped;
	SDL_Quit();
}

}
