#include "../engine.hpp"

#include <cstdint>

#include <iostream>
#include <fstream>
#include <string>

#include "ini.hpp"

#include "../debug.hpp"

#define DEFAULT_GAME_DIR "C:\\Program Files (x86)\\Microsoft Games\\Age of Empires"

namespace aoe {

Config::Config(Engine &e) : Config(e, "") {}

Config::Config(Engine &e, const std::string &s) : e(e), path(s), game_dir(), autostart(false), music_volume(0.3), sfx_volume(0.7) {
#if _WIN32
	game_dir = DEFAULT_GAME_DIR;
#endif
}

Config::~Config() {
	if (path.empty())
		return;

	try {
		save(path);
	} catch (std::exception&) {}
}

void Config::reset() {
	path.clear();
	game_dir.clear();
	autostart = false;
	music_volume = sfx_volume = SDL_MIX_MAXVOLUME;
}

#define EXP_BASE 5.0
#define SCALE SDL_MIX_MAXVOLUME

static int exponential_to_linear(double y)
{
	y = std::clamp(0.0, y, 1.0);
	double x = log(y * (EXP_BASE - 1.0) + 1.0) / log(EXP_BASE);
	return (int)round(x * SCALE);
}

static double linear_to_exponential(double y)
{
	double x = std::clamp<double>(0, y, SCALE) / SCALE;
	return (pow(EXP_BASE, x) - 1.0) / (EXP_BASE - 1.0);
}

void Config::load(const std::string &path) {
	ZoneScoped;
	IniParser ini(path.c_str());
	if (!ini.exists()) {
		perror(path.c_str());
		return;
	}

#if 1
	const char *section = "audio";

#if 0
	double music, sfxvol;
	music = ini.get_or_default(section, "music_volume", 0.3);
	sfxvol = ini.get_or_default(section, "sounds_volume", 0.7);

	music_volume = (uint8_t)(std::clamp(0.0, music, 1.0) / SDL_MIX_MAXVOLUME);
#endif

	Audio &sfx = e.sfx;
	std::string v;

	ini.try_get(section, "music_main_menu", sfx.jukebox[MusicId::menu]);
	ini.try_get(section, "music_victory", sfx.jukebox[MusicId::success]);
	ini.try_get(section, "music_defeat", sfx.jukebox[MusicId::fail]);
	ini.try_get(section, "music_gameplay", sfx.jukebox[MusicId::game]);

	bool enable_music = ini.get_or_default(section, "music_enabled", true);
	bool enable_sound = ini.get_or_default(section, "sounds_enabled", true);

	if (enable_music)
		sfx.unmute_music();
	else
		sfx.mute_music();

	if (enable_sound)
		sfx.unmute_sfx();
	else
		sfx.mute_sfx();

	ini.try_get("legacy", "game_directory", game_dir);
#if _WIN32
	if (game_dir == "") {
		// try to autodetect
		game_dir = DEFAULT_GAME_DIR;
	}
#endif

	autostart = ini.get_or_default("", "autostart", false);
#else
	try {
		json data(json::parse(in));

		music_volume = (uint8_t)std::clamp<int>(data["audio"]["music_volume"], 0, SDL_MIX_MAXVOLUME);
		sfx_volume = (uint8_t)std::clamp<int>(data["audio"]["sounds_volume"], 0, SDL_MIX_MAXVOLUME);
	} catch (std::exception &e) {
		throw std::runtime_error(e.what());
	}
#endif
}

void Config::save(const std::string &path) {
	ZoneScoped;
#if 1
	IniParser ini(NULL);

	ini.add_cache("legacy", "game_directory", game_dir);
	ini.add_cache("", "autostart", autostart);

	const char *section = "audio";
	Audio &sfx = e.sfx;

	ini.add_cache(section, "music_enabled", !sfx.is_muted_music());
	ini.add_cache(section, "sounds_enabled", !sfx.is_muted_sfx());

	ini.add_cache(section, "music_volume", music_volume, 2);
	ini.add_cache(section, "sounds_volume", sfx_volume, 2);

	// TODO add kv pairs
	ini.write_file(path.c_str());
#else
	using json = nlohmann::json;
	json data;

	data["audio"]["music_enabled"]  = !sfx.is_muted_music();
	data["audio"]["sounds_enabled"] = !sfx.is_muted_sfx();
	data["audio"]["music_volume"]   = music_volume;
	data["audio"]["sounds_volume"]  = sfx_volume;

	data["audio"]["music_main_menu"] = sfx.jukebox[MusicId::menu];
	data["audio"]["music_victory"  ] = sfx.jukebox[MusicId::success];
	data["audio"]["music_defeat"   ] = sfx.jukebox[MusicId::fail];
	data["audio"]["music_gameplay" ] = sfx.jukebox[MusicId::game];

	data["autostart"] = autostart;
#endif
}

}
