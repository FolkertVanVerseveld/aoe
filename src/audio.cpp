/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "audio.hpp"

#include "fs.hpp"

#include <SDL2/SDL_rwops.h>

#include <cassert>

#include <mutex>
#include <stdexcept>
#include <string>

namespace genie {

extern std::string game_dir;

const char *msc_names[msc_count] = {
	"",
	"Title",
	"Won",
	"Loss",
	"Sick Sate Rittim",
	"Fretless Salsa",
	"Polyrhythmic Song",
	"String Attack!",
	"Medieval Melody",
	"The Old One Sleeps",
	"Slow and Spacious",
	"Wally",
	"Rain",
};

/*
"cave",
"death",
"battle",
"gamelan",
"party",
"rain",
"hunt",
"thunder",
"wally",
"gray sky",
*/

/** Ensures all mix music calls are thread safe*/
std::recursive_mutex sfx_mut;

Jukebox jukebox;
Mix_Music *music;
unsigned music_game_pos = 1;
int playing;

// don't touch these as they cannot be changed directly
static bool msc_on = true, sfx_on = true;
static int sfx_vol = MIX_MAX_VOLUME, msc_vol = MIX_MAX_VOLUME;

static void music_stop() {
	if (music) {
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		Mix_HookMusicFinished(NULL);
		music = NULL;
		playing = -1;
	}
}

static void music_done() {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	if (playing != 1) {
		music_stop();
		return;
	}

	if (music)
		jukebox.next();
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

Jukebox::Jukebox() : id(MusicId::none), cache() {}
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
	if (!sfx_on)
		return -1;

	auto search = cache.find(id);

	if (search == cache.end()) {
		auto ins = cache.emplace(id, id);
		assert(ins.second);

		return ins.first->second.play(loops, channel);
	} else {
		return search->second.play(loops, channel);
	}
}

void Jukebox::play(MusicId id, int loops) {
	if (!msc_on)
		return;

	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	// Don't restart music if it's already playing.
	if (Mix_PlayingMusic() && this->id == id)
		return;

	music_stop();
	playing = 0;

	if (id == MusicId::none)
		return;

	std::string name;

	switch (id) {
		case MusicId::open: name = "open.mid"; break;
		case MusicId::win: name = "won.mid"; break;
		case MusicId::lost: name = "lost.mid"; break;
		default:
			music_game_pos = (unsigned)id - (unsigned)MusicId::track1 + 1;
			name = "music" + std::to_string(music_game_pos) + ".mid";
			break;
	}

	assert(!name.empty());

	std::string path(genie::msc_path(genie::game_dir, name));

	if (!(music = Mix_LoadMUS(path.c_str()))) {
		this->id = MusicId::none;
		playing = -1;
		return;
	}

	Mix_HookMusicFinished(music_done);
	if (Mix_PlayMusic(music, loops) != -1) {
		this->id = id;
		playing = 1;
	}
}

void Jukebox::next() {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);

	music_stop();
	playing = 0;

	if (!msc_on || id < MusicId::game)
		return;

	if (++music_game_pos >= 10)
		music_game_pos = 1;
	else if (music_game_pos < 1)
		music_game_pos = 1;

	std::string path(genie::msc_path(genie::game_dir, "music" + std::to_string(music_game_pos) + ".mid"));

	if ((music = Mix_LoadMUS(path.c_str())) != NULL) {
		Mix_HookMusicFinished(music_done);
		Mix_PlayMusic(music, 0);
		this->id = (MusicId)((unsigned)MusicId::game + music_game_pos - 1);
		playing = 1;
	}
}

void Jukebox::stop(int channel) {
	std::lock_guard<std::recursive_mutex> lock(sfx_mut);
	Mix_HaltChannel(channel);
}

int Jukebox::sfx_volume() const noexcept { return sfx_vol; }
int Jukebox::msc_volume() const noexcept { return msc_vol; }

int Jukebox::sfx_volume(int v, int ch) {
	if (ch == -1)
		sfx_vol = v;
	return Mix_Volume(ch, v);
}

int Jukebox::msc_volume(int v) {
	msc_vol = v;
	return Mix_VolumeMusic(v);
}

bool Jukebox::sfx_enabled() const noexcept { return sfx_on; }

bool Jukebox::sfx(bool v) {
	if (!v) stop(-1);
	return sfx_on = v;
}

bool Jukebox::msc_enabled() const noexcept { return msc_on; }

bool Jukebox::msc(bool v) {
	if (!v) stop();
	return msc_on = v;
}

}
