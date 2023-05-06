#pragma once

#if __APPLE__
#include "/usr/local/Cellar/sdl2_mixer/2.0.4_1/include/SDL2/SDL_mixer.h"
#else
#include <SDL2/SDL_mixer.h>
#endif

#include <cstddef>

#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <optional>
#include <vector>

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

enum class SfxId {
	// TODO /sfx_//g
	sfx_ui_click,
	sfx_chat,
	towncenter,
	barracks,
	bld_die_random,
	bld_die1,
	bld_die2,
	bld_die3,
	player_resign,
	gameover_defeat,
	gameover_victory,
	villager_random,
	villager1,
	villager2,
	villager3,
	villager4,
	villager5,
	villager6,
	villager7,
	villager_die_random,
	villager_die1,
	villager_die2,
	villager_die3,
	villager_die4,
	villager_die5,
	villager_die6,
	villager_die7,
	villager_die8,
	villager_die9,
	villager_die10,
	villager_attack_random,
	villager_attack1,
	villager_attack2,
	villager_attack3,
	villager_spawn,
	wood_worker_attack,
	melee_spawn,
	priest,
	priest_attack_random,
	priest_attack1,
	priest_attack2,
};

class Audio final {
	int freq, channels;
	Uint16 format;

	std::unique_ptr<Mix_Music, decltype(&Mix_FreeMusic)> music;
	bool music_mute, sfx_mute;
	std::string music_file;

	std::mutex m_mix;
	std::map<TauntId, std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)>> taunts;
	std::map<SfxId, std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)>> sfx;
public:
	std::map<MusicId, std::string> jukebox;

	bool play_taunts;

	Audio();
	~Audio();

	/** Force stop all audio playing. */
	void panic();
	/** Force stop all audio playing and clear audio caches. */
	void reset();

	void play_music(const char *file, int loops=0);
	void play_music(MusicId id, int loops=0);

	void stop_music();

	void mute_music();
	void unmute_music();

	void mute_sfx();
	void unmute_sfx();

	constexpr bool is_muted_music() const noexcept { return music_mute; }
	constexpr bool is_muted_sfx() const noexcept { return sfx_mute; }

	void load_taunt(TauntId id, const char *file);
	void load_taunt(TauntId id, const std::vector<uint8_t> &data);
	void play_taunt(TauntId id);

	/** Check if text should trigger taunt sound effect. */
	std::optional<TauntId> is_taunt(const std::string&);

	void load_sfx(SfxId id, const std::vector<uint8_t> &data);
	void play_sfx(SfxId id, int loops=0);
};

}
