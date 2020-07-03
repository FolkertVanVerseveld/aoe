/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "audio.hpp"

#include <SDL2/SDL_rwops.h>

#include <cassert>

#include <mutex>
#include <stdexcept>
#include <string>

namespace genie {

/** Ensures all mix music calls are thread safe*/
std::recursive_mutex sfx_mut;

Jukebox jukebox;
Mix_Music *music;
unsigned music_game_pos = 1;
int sfx_vol = MIX_MAX_VOLUME;

static void music_done() {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	if (music)
		jukebox.next();
}

static void music_stop() {
	if (music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		Mix_HookMusicFinished(NULL);
		music = NULL;
	}
}

Sfx::Sfx(DrsId id) : Sfx(id, NULL) {
	DrsItem item;
	Mix_Chunk *clip;

	if (!genie::assets.blobs[2]->open_item(item, (res_id)id, DrsType::wave) &&
		!genie::assets.blobs[3]->open_item(item, (res_id)id, DrsType::wave))
		throw std::runtime_error(std::string("Could not load sound ") + std::to_string((unsigned)id));

	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	assert(item.size >= 0 && item.size <= INT_MAX);
	if (!(clip = Mix_LoadWAV_RW(SDL_RWFromMem(item.data, (int)item.size), 1)))
		throw std::runtime_error(std::string("Could not load sound ") + std::to_string((unsigned)id) + ": " + Mix_GetError());

	this->clip.reset(clip);
}

Sfx::Sfx(DrsId id, Mix_Chunk *clip) : id(id), clip(clip, &Mix_FreeChunk), loops(0), channel(-1) {}

int Sfx::play(int loops, int channel) {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	return Mix_PlayChannel(channel, clip.get(), loops);
}

Jukebox::Jukebox() : playing(0), id(MusicId::none), cache() {}
Jukebox::~Jukebox() { close(); }

void Jukebox::close() {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	stop(-1);
	stop();
	cache.clear();
}

int Jukebox::is_playing(MusicId &id) {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	id = this->id;
	return playing;
}

void Jukebox::stop() {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	music_stop();
	playing = 0;
}

int Jukebox::sfx(DrsId id, int loops, int channel) {
	auto search = cache.find(id);

	if (search == cache.end()) {
		auto ins = cache.emplace(id, id);
		assert(ins.second);

		return ins.first->second.play(loops, channel);
	} else {
		return search->second.play(loops, channel);
	}
}

void Jukebox::stop(int channel) {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	Mix_HaltChannel(channel);
}

int Jukebox::sfx_volume() const noexcept { return sfx_vol; }

int Jukebox::sfx_volume(int v, int ch) {
	sfx_vol = v;
	return Mix_Volume(ch, v);
}

}
