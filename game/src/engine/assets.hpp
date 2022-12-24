#pragma once

#include "../legacy.hpp"

#include <map>

namespace aoe {

class Assets final {
	std::map<io::DrsId, std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)>> pals;
public:
	Assets();

	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> open_pal(io::DrsId id);
};

}
