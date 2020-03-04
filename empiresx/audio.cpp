/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "audio.hpp"

#include "engine.hpp"

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

Sfx::Sfx(SfxId id) : Sfx(id, NULL) {
	void *data;
	size_t count;
	Mix_Chunk *clip;

	// special case for multiplayer auto-messages
	if (id >= SfxId::taunt1 && id <= SfxId::taunt25) {
		std::lock_guard<std::recursive_mutex> lock(sfx_mut);

#pragma warning(push)
#pragma warning(disable: 4996)

		char buf[32];
		sprintf(buf, "taunt%03u.wav", (unsigned)id - (unsigned)SfxId::taunt1 + 1);

#pragma warning(pop)

		if (!(clip = fs.open_wav(buf)))
			throw std::runtime_error(std::string("Could not load ") + buf);

		this->clip.reset(clip);
		return;
	}

	if (!(data = eng->assets->open_wav((res_id)id, count)))
		throw std::runtime_error(std::string("Could not load sound ") + std::to_string((unsigned)id));

	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	assert(count >= 0 && count <= INT_MAX);
	if (!(clip = Mix_LoadWAV_RW(SDL_RWFromMem(data, (int)count), 1)))
		throw std::runtime_error(std::string("Could not load sound ") + std::to_string((unsigned)id) + ": " + Mix_GetError());

	this->clip.reset(clip);
}

Sfx::Sfx(SfxId id, Mix_Chunk *clip) : id(id), clip(clip, &Mix_FreeChunk), loops(0), channel(-1) {}

void Sfx::play(int loops, int channel) {
	if (!eng->config().sound)
		return;

	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	Mix_PlayChannel(channel, clip.get(), loops);
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

void Jukebox::sfx(SfxId id, int loops, int channel) {
	auto search = cache.find(id);

	if (search == cache.end()) {
		auto ins = cache.emplace(id, id);
		assert(ins.second);

		ins.first->second.play(loops, channel);
	} else {
		search->second.play(loops, channel);
	}
}

void Jukebox::play(MusicId id) {
	if (!eng->config().music)
		return;

	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	music_stop();
	playing = 0;

	if (id == MusicId::none)
		return;

	std::string name;

	switch (id) {
	case MusicId::start:
		name = "open.mid";
		break;
	case MusicId::win:
		name = "win.mid";
		break;
	case MusicId::lost:
		name = "lost.mid";
		break;
	case MusicId::game:
		name = "music1.mid";
		break;
	}

	assert(!name.empty());

	if ((music = fs.open_msc(name)) != NULL) {
		Mix_HookMusicFinished(music_done);
		Mix_PlayMusic(music, 0);
		this->id = id;
		playing = 1;
	} else {
		playing = -1;
	}
}

void Jukebox::stop(int channel) {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	Mix_HaltChannel(channel);
}

void Jukebox::next() {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	music_stop();
	playing = 0;

	if (!eng->config().music || id != MusicId::game)
		return;

	if (++music_game_pos >= 10)
		music_game_pos = 1;
	else if (music_game_pos < 1)
		music_game_pos = 1;

	std::string path("music" + std::to_string(music_game_pos) + ".mid");

	if ((music = fs.open_msc(path)) != NULL) {
		Mix_HookMusicFinished(music_done);
		Mix_PlayMusic(music, 0);
		this->id = id;
		playing = 1;
	}
}

}
