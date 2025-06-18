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

void Config::load(const std::string &path) {
	ZoneScoped;
	IniParser ini(path.c_str());
	if (!ini.exists()) {
		perror(path.c_str());
		return;
	}

	const char *section = "audio";
	Audio &sfx = e.sfx;

	ini.try_get(section, "music_main_menu", sfx.jukebox[MusicId::menu]);
	ini.try_get(section, "music_victory", sfx.jukebox[MusicId::success]);
	ini.try_get(section, "music_defeat", sfx.jukebox[MusicId::fail]);
	ini.try_get(section, "music_gameplay", sfx.jukebox[MusicId::game]);

	sfx.load(ini);

	ini.try_get("legacy", "game_directory", game_dir);
#if _WIN32
	if (game_dir == "") {
		// try to autodetect
		game_dir = DEFAULT_GAME_DIR;
	}
#endif

	autostart = ini.get_or_default("", "autostart", false);
}

void Config::save(const std::string &path) {
	ZoneScoped;
	IniParser ini(NULL);

	ini.add_cache("legacy", "game_directory", game_dir);
	ini.add_cache("", "autostart", autostart);

	const char *section = "audio";
	Audio &sfx = e.sfx;

	ini.add_cache(section, "music_enabled", sfx.is_enabled_music());
	ini.add_cache(section, "sounds_enabled", sfx.is_enabled_sound());

	ini.add_cache(section, "music_volume", sfx.get_music_volume(), 2);
	ini.add_cache(section, "sounds_volume", sfx.get_sound_volume(), 2);

	ini.add_cache(section, "music_main_menu", sfx.jukebox[MusicId::menu]);
	ini.add_cache(section, "music_victory", sfx.jukebox[MusicId::success]);
	ini.add_cache(section, "music_defeat", sfx.jukebox[MusicId::fail]);
	ini.add_cache(section, "music_gameplay", sfx.jukebox[MusicId::game]);

	ini.write_file(path.c_str());
}

}
