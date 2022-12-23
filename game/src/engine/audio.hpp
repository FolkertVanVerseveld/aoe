#pragma once

#if __APPLE__
#include "/usr/local/Cellar/sdl2_mixer/2.0.4_1/include/SDL2/SDL_mixer.h"
#else
#include <SDL2/SDL_mixer.h>
#endif

#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <optional>

namespace aoe {

enum class MusicId {
	menu,
	success,
	fail,
	game,
};

enum class TauntId {
	yes,
	no,
	food,
	wood,
	gold,
	stone,
	ehh,
	nice_try,
	yay,
	in_town,
	owaah,
	join,
	disagree,
	start,
	alpha,
	attack,
	haha,
	mercy,
	hahaha,
	satisfaction,
	nice,
	wtf,
	get_out,
	let_go, //???
	yeah,
	max,
};

class Audio final {
	int freq, channels;
	Uint16 format;

	std::unique_ptr<Mix_Music, decltype(&Mix_FreeMusic)> music;
	bool music_mute;
	std::string music_file;

	std::mutex m_mix;
	std::map<TauntId, std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)>> taunts;
public:
	std::map<MusicId, std::string> jukebox;

	Audio();
	~Audio();

	void play_music(const char *file, int loops=0);
	void play_music(MusicId id);

	void stop_music();

	void mute_music();
	void unmute_music();

	constexpr bool is_muted_music() const noexcept { return music_mute; }

	void load_taunt(TauntId id, const char *file);
	void play_taunt(TauntId id);

	/** Check if text should trigger taunt sound effect. */
	std::optional<TauntId> is_taunt(const std::string&);
};

}
