#pragma once

#if __APPLE__
#include "/usr/local/Cellar/sdl2_mixer/2.0.4_1/include/SDL2/SDL_mixer.h"
#else
#include <SDL2/SDL_mixer.h>
#endif

#include <memory>

namespace aoe {

class Audio final {
	int freq, channels;
	Uint16 format;

	std::unique_ptr<Mix_Music, decltype(&Mix_FreeMusic)> music;
public:
	Audio();
	~Audio();

	void play_music(const char *file, int loops=0);
};

}
