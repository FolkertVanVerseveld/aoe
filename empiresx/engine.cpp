#include "engine.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <stdexcept>

SDL::SDL() {
	if (SDL_Init(SDL_INIT_VIDEO))
		throw std::runtime_error(std::string("Unable to initialize SDL: ") + SDL_GetError());
}

SDL::~SDL() {
	SDL_Quit();
};

IMG::IMG() {
	if (IMG_Init(0) == -1)
		throw std::runtime_error(std::string("Unable to initialize SDL_Image: ") + IMG_GetError());
}

IMG::~IMG() {
	IMG_Quit();
}

MIX::MIX() {
	int frequency = 44100;
	Uint32 format = MIX_DEFAULT_FORMAT;
	int channels = 2;
	int chunksize = 1024;

	if (Mix_Init(0) == -1)
		throw std::runtime_error(std::string("Unable to initialize SDL_Mixer: ") + Mix_GetError());

	if (Mix_OpenAudio(frequency, format, channels, chunksize))
		throw std::runtime_error(std::string("Unable to open audio device: ") + Mix_GetError());
}

MIX::~MIX() {
	Mix_Quit();
}

TTF::TTF() {
	if (TTF_Init())
		throw std::runtime_error(std::string("Unable to initialize SDL_ttf: ") + TTF_GetError());
}

TTF::~TTF() {
	TTF_Quit();
}

Engine::Engine() {}
Engine::~Engine() {}