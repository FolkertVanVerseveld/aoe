#include "../engine.hpp"

#include <cstdint>

#include <iostream>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include "../debug.hpp"

namespace aoe {

static void read(std::ifstream &in, SDL_Rect &r) {
	static_assert(sizeof(int) == sizeof(uint32_t));
	in.read((char*)&r.x, sizeof(r.x));
	in.read((char*)&r.y, sizeof(r.y));
	in.read((char*)&r.w, sizeof(r.w));
	in.read((char*)&r.h, sizeof(r.h));
}

static void read(std::ifstream &in, std::string &s, uint32_t max=UINT32_MAX) {
	uint32_t n;
	in.read((char*)&n, sizeof(n));

	if (n > max)
		throw std::runtime_error("string overflow");

	s.resize(n, ' ');
	in.read(s.data(), n);
}

static void write(std::ofstream &out, SDL_Rect r) {
	static_assert(sizeof(int) == sizeof(int32_t));
	out.write((const char*)&r.x, sizeof r.x);
	out.write((const char*)&r.y, sizeof r.y);
	out.write((const char*)&r.w, sizeof r.w);
	out.write((const char*)&r.h, sizeof r.h);
}

static void write(std::ofstream &out, const std::string &s) {
	if (s.size() > UINT32_MAX)
		throw std::runtime_error("string overflow");

	uint32_t n = s.size();
	out.write((const char*)&n, sizeof(n));
	out.write(s.data(), n);
}

Config::Config(Engine &e) : Config(e, "") {}
Config::Config(Engine &e, const std::string &s) : e(e), bnds{ 0, 0, 1, 1 }, display{ 0, 0, 1, 1 }, vp{ 0, 0, 1, 1 }, path(s), game_dir(), autostart(false), music_volume(SDL_MIX_MAXVOLUME), sfx_volume(SDL_MIX_MAXVOLUME) {}

Config::~Config() {
	if (path.empty())
		return;

	try {
		save(path);
	} catch (std::exception&) {}
}

void Config::reset() {
	vp = display = bnds = SDL_Rect{0, 0, 1, 1};
	path.clear();
	game_dir.clear();
	autostart = false;
	music_volume = sfx_volume = SDL_MIX_MAXVOLUME;
}

void Config::load(const std::string &path) {
	ZoneScoped;
	using json = nlohmann::json;

	std::ifstream in(path);// , std::ios_base::binary);

	// let c++ take care of any errors
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		json data(json::parse(in));

		bnds.x = data["game_window"]["x"];
		bnds.y = data["game_window"]["y"];
		bnds.w = data["game_window"]["w"];
		bnds.h = data["game_window"]["h"];

		display.x = data["display"]["x"];
		display.y = data["display"]["y"];
		display.w = data["display"]["w"];
		display.h = data["display"]["h"];

		vp.x = data["viewport"]["x"];
		vp.y = data["viewport"]["y"];
		vp.w = data["viewport"]["w"];
		vp.h = data["viewport"]["h"];

		bool mute_music = !data["audio"]["music_enabled"];
		bool mute_sfx   = !data["audio"]["sounds_enabled"];

		music_volume = (uint8_t)std::clamp<int>(data["audio"]["music_volume"], 0, SDL_MIX_MAXVOLUME);
		sfx_volume = (uint8_t)std::clamp<int>(data["audio"]["sounds_volume"], 0, SDL_MIX_MAXVOLUME);

		Audio &sfx = e.sfx;

		if (mute_music)
			sfx.mute_music();
		else
			sfx.unmute_music();

		if (mute_sfx)
			sfx.mute_sfx();
		else
			sfx.unmute_sfx();

		sfx.jukebox[MusicId::menu   ] = data["audio"]["music_main_menu"];
		sfx.jukebox[MusicId::success] = data["audio"]["music_victory"  ];
		sfx.jukebox[MusicId::fail   ] = data["audio"]["music_defeat"   ];
		sfx.jukebox[MusicId::game   ] = data["audio"]["music_gameplay" ];

		this->autostart = data["autostart"];

		game_dir = data["original_game_directory"];
	} catch (std::exception &e) {
		throw std::runtime_error(e.what());
	}
}

void Config::save(const std::string &path) {
	ZoneScoped;
	using json = nlohmann::json;
	json data;

	data["game_window"]["x"] = bnds.x;
	data["game_window"]["y"] = bnds.y;
	data["game_window"]["w"] = bnds.w;
	data["game_window"]["h"] = bnds.h;

	data["display"]["x"] = display.x;
	data["display"]["y"] = display.y;
	data["display"]["w"] = display.w;
	data["display"]["h"] = display.h;

	data["viewport"]["x"] = vp.x;
	data["viewport"]["y"] = vp.y;
	data["viewport"]["w"] = vp.w;
	data["viewport"]["h"] = vp.h;

	Audio &sfx = e.sfx;

	data["audio"]["music_enabled"]  = !sfx.is_muted_music();
	data["audio"]["sounds_enabled"] = !sfx.is_muted_sfx();
	data["audio"]["music_volume"]   = music_volume;
	data["audio"]["sounds_volume"]  = sfx_volume;

	data["audio"]["music_main_menu"] = sfx.jukebox[MusicId::menu];
	data["audio"]["music_victory"  ] = sfx.jukebox[MusicId::success];
	data["audio"]["music_defeat"   ] = sfx.jukebox[MusicId::fail];
	data["audio"]["music_gameplay" ] = sfx.jukebox[MusicId::game];

	data["autostart"] = autostart;

	data["original_game_directory"] = e.game_dir;
	std::ofstream out(path);// , std::ios_base::binary | std::ios_base::trunc);

	// let c++ take care of any errors
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	out << data.dump(4) << std::endl;
}

}
