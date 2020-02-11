#pragma once

#include <SDL2/SDL_mixer.h>

#include <memory>
#include <map>

namespace genie {

enum class SfxId {
	none = 0
};

enum class MusicId {
	none,
	start,
	win,
	lost,
	game // track1-10
};

class Sfx final {
	SfxId id;
	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> clip;
	int loops, channel;
public:
	Sfx(SfxId id);
	Sfx(SfxId id, Mix_Chunk *clip);

	void play(int loops=0, int channel=-1);

	friend bool operator<(const Sfx &lhs, const Sfx &rhs) { return lhs.id < rhs.id; }
};

extern class Jukebox final {
	int playing;
	MusicId id;
	std::map<SfxId, Sfx> cache;
public:
	Jukebox();
	~Jukebox();

	void close();

	int is_playing(MusicId &id); /**< 1=true, 0=false, -1=error */

	void sfx(SfxId id, int loops=0, int channel=-1);

	void play(MusicId id);
	void stop();
	void stop(int channel);

	void next();
} jukebox;

}
