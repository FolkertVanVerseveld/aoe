/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "drs.hpp"

#include <SDL2/SDL_mixer.h>

#include <memory>
#include <map>

namespace genie {

class Sfx final {
	DrsId id;
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> clip;
	int loops, channel;
public:
	Sfx(DrsId id);
	Sfx(DrsId id, Mix_Chunk *clip);
	Sfx(const Sfx&) = delete;

	int play(int loops=0, int channel=-1);

	friend bool operator<(const Sfx &lhs, const Sfx &rhs) { return lhs.id < rhs.id; }
};

enum class MusicId {
	none,
	start,
	win,
	lost,
	game // track1-10
};

static constexpr int sfx_max_volume = MIX_MAX_VOLUME;

extern class Jukebox final {
	int playing;
	MusicId id;
	std::map<DrsId, Sfx> cache;
public:
	Jukebox();
	~Jukebox();

	void close();

	int is_playing(MusicId &id); /**< 1=true, 0=false, -1=error */
	int sfx(DrsId id, int loops=0, int channel=-1);
	int play(MusicId id);
	void stop();
	void stop(int channel);

	int sfx_volume() const noexcept;
	int sfx_volume(int v, int ch=-1);

	void next();
} jukebox;

}
