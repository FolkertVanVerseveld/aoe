#pragma once

#include <string>

#include "gfx.hpp"

namespace aoe {

class Engine;

class Background final {
public:
	io::DrsBkg drs;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal;
	gfx::Image img;

	Background();
};

class Assets final {
public:
	std::string path;
	Background bkg_main;

	Assets(int id, Engine &e, const std::string &path);
};

}
