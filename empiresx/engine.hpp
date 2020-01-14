#pragma once

#include "font.hpp"

namespace genie {

//// SDL subsystems ////

class SDL final {
public:
	SDL();
	~SDL();
};

class IMG final {
public:
	IMG();
	~IMG();
};

class MIX final {
public:
	MIX();
	~MIX();
};

//// Engine stuff ////

class Engine final {
	SDL sdl;
	IMG img;
	MIX mix;
	TTF ttf;
public:
	Engine();
	~Engine();
};

}