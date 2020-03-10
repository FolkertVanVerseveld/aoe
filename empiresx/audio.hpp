/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include <SDL2/SDL_mixer.h>

#include <memory>
#include <map>

namespace genie {

enum class SfxId {
	/* User interface */
	button4 = 50300,
	chat = 50302,
	error = 50303,
	win = 50320,
	lost = 50321,
	priest_converted = 50313,
	priest_convert_reverse = 50314,

	game = 5036,

	priest_convert1 = 5050,
	priest_convert2 = 5051,
	/* Buildings */
	barracks = 5022,
	town_center = 5044,
	academy = 5002,
	/* Villagers */
	villager_move = 5006,
	villager1 = 5037,
	villager2 = 5053,
	villager3 = 5054,
	villager4 = 5158,
	villager5 = 5171,

	villager_destroy1 = 5055,
	villager_destroy2 = 5056,
	villager_destroy3 = 5057,
	villager_destroy4 = 5058,
	villager_destroy5 = 5059,
	villager_destroy6 = 5060,
	villager_destroy7 = 5061,
	villager_destroy8 = 5062,
	villager_destroy9 = 5063,
	villager_destroy10 = 5064,
	/* Buildings */
	building_destroy1 = 5077,
	building_destroy2 = 5078,
	building_destroy3 = 5079,
	/* Placeholders */
	tribe = 5231,
	tribe_destroy1 = 5102,
	tribe_destroy2 = 5103,
	tribe_destroy3 = 5104,
	/* 
	Multiplayer taunts
	It is important to note that the following IDs do not really correspond with the real sound,
	but we are just recycling some bina IDs that are invalid SfxIds anyway.
	*/
	taunt1 = 50001,
	taunt2 = 50002,
	taunt3 = 50003,
	taunt4 = 50004,
	taunt5 = 50005,
	taunt6 = 50006,
	taunt7 = 50007,
	taunt8 = 50008,
	taunt9 = 50009,
	taunt10 = 50010,
	taunt11 = 50011,
	taunt12 = 50012,
	taunt13 = 50013,
	taunt14 = 50014,
	taunt15 = 50015,
	taunt16 = 50016,
	taunt17 = 50017,
	taunt18 = 50018,
	taunt19 = 50019,
	taunt20 = 50020,
	taunt21 = 50021,
	taunt22 = 50022,
	taunt23 = 50023,
	taunt24 = 50024,
	taunt25 = 50025,

	// various
	unit_select = 5231,
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
