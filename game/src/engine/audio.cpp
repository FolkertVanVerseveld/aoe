#include "audio.hpp"

#include <stdexcept>
#include <string>

#define SAMPLING_FREQ 48000
#define NUM_CHANNELS 2

namespace aoe {

Audio::Audio() : freq(0), channels(0), format(0), music(nullptr, Mix_FreeMusic), jukebox(), music_mute(false) {
	int flags = MIX_INIT_MP3;

	if ((Mix_Init(flags) & flags) != flags)
		throw std::runtime_error(std::string("Mix: Could not initialize audio: ") + Mix_GetError());

	if (Mix_OpenAudio(SAMPLING_FREQ, MIX_DEFAULT_FORMAT, NUM_CHANNELS, 1024) == -1)
		throw std::runtime_error(std::string("Mix: Could not initialize audio: ") + Mix_GetError());

	if (Mix_QuerySpec(&freq, &format, &channels))
		printf("freq=%d, channels=%d, format=0x%X\n", freq, channels, format);
}

Audio::~Audio() {
	music.reset();
	Mix_Quit();
}

void Audio::play_music(const char *file, int loops) {
	// always load music even if muted. so when we unmute, it should just work
	music.reset(Mix_LoadMUS(file));
	if (!music.get()) {
		fprintf(stderr, "%s: could not load music: %s\n", __func__, Mix_GetError());
		return;
	}
	music_file = file;

	if (!music_mute)
		Mix_PlayMusic(music.get(), loops);
}

void Audio::play_music(MusicId id) {
	auto it = jukebox.find(id);
	if (it == jukebox.end()) {
		fprintf(stderr, "%s: cannot play music id %d: not found\n", __func__, id);
		return;
	}

	// ignore if already playing
	if (it->second != music_file || !Mix_PlayingMusic())
		play_music(it->second.c_str());
}

void Audio::stop_music() {
	Mix_HaltMusic();
}

void Audio::mute_music() {
	if (!music_mute)
		stop_music();

	music_mute = true;
}

void Audio::unmute_music() {
	if (!music_mute)
		return;

	music_mute = false;
	// restart if music is loaded
	if (music.get())
		Mix_PlayMusic(music.get(), 0);
}

}
