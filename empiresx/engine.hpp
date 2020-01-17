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

class Engine;

extern Engine *eng;

enum class ConfigScreenMode {
	MODE_640_480,
	MODE_800_600,
	MODE_1024_768,
	MODE_FULLSCREEN,
	MODE_CUSTOM,
};

class Config final {
public:
	/** Startup configuration specified by program arguments. */
	ConfigScreenMode scrmode;
	unsigned poplimit = 50;

	static constexpr unsigned POP_MIN = 25, POP_MAX = 200;

	Config(int argc, char* argv[]);
};

class Assets final {
public:
	Font fnt_default;

	Assets();
};

class Engine final {
	Config& cfg;
	SDL sdl;
	IMG img;
	MIX mix;
	TTF ttf;
public:
	std::unique_ptr<Assets> assets;

	Engine(Config &cfg);
	~Engine();

	void show_error(const std::string &title, const std::string& str);
	void show_error(const std::string& str);
};

}