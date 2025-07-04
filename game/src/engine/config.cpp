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

Config::Config(Engine &e, const std::string &s)
	: e(e), path(s), game_dir(), username()
	, autostart(false), music_volume(0.3), sfx_volume(0.7)
{
#if _WIN32
	game_dir = DEFAULT_GAME_DIR;
#endif
}

Config::~Config() {
	if (!path.empty())
		save(path);
}

void Config::reset() {
	path.clear();
	game_dir.clear();
	autostart = false;
	music_volume = sfx_volume = SDL_MIX_MAXVOLUME;
}

ipStatus Config::load(const std::string &path) {
	ZoneScoped;
	IniParser ini(path.c_str());
	if (!ini.exists()) {
		perror(path.c_str());
		return ipStatus::not_found;
	}

	ini.try_get("legacy", "game_directory", game_dir);
#if _WIN32
	if (game_dir == "") {
		// try to autodetect
		game_dir = DEFAULT_GAME_DIR;
	}
#endif

	autostart = ini.get_or_default("", "autostart", false);
	ini.try_get("", "username", username);


	const char *section = "audio";
	Audio &sfx = e.sfx;

	ini.try_get(section, "music_main_menu", sfx.jukebox[MusicId::menu]);
	ini.try_get(section, "music_victory", sfx.jukebox[MusicId::success]);
	ini.try_get(section, "music_defeat", sfx.jukebox[MusicId::fail]);
	ini.try_get(section, "music_gameplay", sfx.jukebox[MusicId::game]);

	sfx.load(ini);

	section = "font";

	static const double pt_min = 5, pt_max = 72;

#define fnt_load(key, f) do {if (ini.try_clamp(section, key, v, pt_min, pt_max) == ipStatus::ok) fnt.f.pt = v; } while(0)

	double v;
	fnt_load("default_pt", arial);
	fnt_load("title_pt", copper2);
	fnt_load("heading_pt", copper);

	return ipStatus::ok;
}

void Config::save(const std::string &path) {
	ZoneScoped;
	IniParser ini(NULL);

	ini.add_cache("legacy", "game_directory", game_dir);
	ini.add_cache("", "autostart", autostart);
	ini.add_cache("", "username", username);

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

	section = "font";

	ini.add_cache(section, "default_pt", fnt.arial.pt, 1);
	ini.add_cache(section, "default_path", fnt.arial.path);
	ini.add_cache(section, "title_pt", fnt.copper2.pt, 1);
	ini.add_cache(section, "title_path", fnt.copper2.path);
	ini.add_cache(section, "heading_pt", fnt.copper.pt, 1);
	ini.add_cache(section, "heading_path", fnt.copper.path);

	ini.write_file(path.c_str());
}

}
