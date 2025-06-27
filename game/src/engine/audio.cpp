#include "audio.hpp"

#include <cctype>
#include <cmath> // log, round
#include <algorithm> // clamp

#include <array>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../debug.hpp"
#include "ini.hpp"

#define SAMPLING_FREQ 48000
#define NUM_CHANNELS 2

namespace aoe {

static const double exp_base = 5.0;
static const double scale = SDL_MIX_MAXVOLUME;

static inline int exponential_to_linear(double y)
{
	y = std::clamp(0.0, y, 1.0);
	double x = log(y * (exp_base - 1.0) + 1.0) / log(exp_base);
	return std::clamp(0, (int)round(x * scale), SDL_MIX_MAXVOLUME);
}

static inline double linear_to_exponential(double y)
{
	double x = std::clamp<double>(0, y, scale) / scale;
	return (pow(exp_base, x) - 1.0) / (exp_base - 1.0);
}

Audio::Audio() : freq(0), channels(0), format(0), music(nullptr, Mix_FreeMusic)
	, enable_music(true), enable_sound(true)
	, music_vol(0.3f), sfx_vol(0.5f)
	, music_file()
	, m_mix(), taunts(), sfx(), jukebox(), play_taunts(true)
{
	int flags = MIX_INIT_MP3;
	int ret;

	if (((ret = Mix_Init(flags)) & flags) != flags) {
		const char *msg = Mix_GetError();
		if (msg)
			throw std::runtime_error(std::string("Mix: Could not initialize audio: ") + msg);
		else
			throw std::runtime_error(std::string("Mix: Could not initialize audio: unknown error"));
	}

	if (Mix_OpenAudio(SAMPLING_FREQ, MIX_DEFAULT_FORMAT, NUM_CHANNELS, 1024) == -1)
		throw std::runtime_error(std::string("Mix: Could not initialize audio: ") + Mix_GetError());

	if (Mix_QuerySpec(&freq, &format, &channels))
		printf("freq=%d, channels=%d, format=0x%X\n", freq, channels, format);

	Mix_AllocateChannels(32);
}

Audio::~Audio() {
	music.reset();
	Mix_Quit();
}

void Audio::load(IniParser &ini) {
	const char *section = "audio";

	music_vol = std::clamp(0.0, ini.get_or_default(section, "music_volume", 0.3), 1.0);
	sfx_vol = std::clamp(0.0, ini.get_or_default(section, "sounds_volume", 0.5), 1.0);

	set_music_volume(music_vol);
	set_sound_volume(sfx_vol);

	enable_music = ini.get_or_default(section, "music_enabled", true);
	enable_sound = ini.get_or_default(section, "sounds_enabled", true);

	set_enable_music(enable_music);
	set_enable_sound(enable_sound);
}

void Audio::set_music_volume(double v) {
	music_vol = v;
	int vol = exponential_to_linear(v);
	Mix_VolumeMusic(vol);
}

void Audio::set_sound_volume(double v) {
	sfx_vol = v;
	int vol = exponential_to_linear(sfx_vol);
	Mix_Volume(-1, vol);
}

void Audio::panic() {
	std::lock_guard<std::mutex> lk(m_mix);
	Mix_HaltMusic();
	Mix_HaltChannel(-1);
}

void Audio::reset() {
	panic();

	taunts.clear();
	sfx.clear();
}

void Audio::play_music(const char *file, int loops) {
	// file may be empty if not configured, just ignore
	if (!file || !*file)
		return;

	// always load music even if muted. so when we unmute, it should just work
	music.reset(Mix_LoadMUS(file));
	if (!music.get()) {
		fprintf(stderr, "%s: could not load music: %s\n", __func__, Mix_GetError());
		return;
	}
	music_file = file;

	if (enable_music)
		Mix_PlayMusic(music.get(), loops);
}

void Audio::play_music(MusicId id, int loops) {
	auto it = jukebox.find(id);
	if (it == jukebox.end()) {
		fprintf(stderr, "%s: cannot play music id %d: not found\n", __func__, id);
		return;
	}

	// ignore if already playing
	if (it->second != music_file || !Mix_PlayingMusic())
		play_music(it->second.c_str(), loops);
}

void Audio::stop_music() {
	Mix_HaltMusic();
}

void Audio::set_enable_music(bool enable) {
	std::lock_guard<std::mutex> lk(m_mix);

	if (enable) {
		if (enable_music)
			return;

		enable_music = true;
		// restart if music is loaded
		if (music.get())
			Mix_PlayMusic(music.get(), 0);
	} else {
		if (enable_music)
			stop_music();

		enable_music = false;
	}
}

void Audio::set_enable_sound(bool enable) {
	std::lock_guard<std::mutex> lk(m_mix);

	if (enable) {
		enable_sound = true;
	} else {
		if (!enable_sound)
			return;

		Mix_HaltChannel(-1);
		enable_sound = false;
	}
}

void Audio::load_taunt(TauntId id, const char *file) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_mix);

	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> sfx(Mix_LoadWAV(file), Mix_FreeChunk);
	if (!sfx.get())
		throw std::runtime_error(std::string("cannot load ") + file + ": " + Mix_GetError());

	taunts.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(sfx.release(), Mix_FreeChunk));
}

void Audio::load_taunt(TauntId id, const std::vector<uint8_t> &data) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_mix);

	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> sfx(Mix_LoadWAV_RW(SDL_RWFromConstMem(data.data(), data.size()), 1), Mix_FreeChunk);
	if (!sfx.get())
		throw std::runtime_error(std::string("cannot load taunt ID ") + std::to_string((unsigned)id) + ": " + Mix_GetError());

	taunts.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(sfx.release(), Mix_FreeChunk));
}

void Audio::play_taunt(TauntId id) {
	ZoneScoped;

	if (!play_taunts)
		return;

	std::lock_guard<std::mutex> lk(m_mix);

	auto it = taunts.find(id);
	if (it == taunts.end()) {
		fprintf(stderr, "%s: cannot play taunt id %d: not found\n", __func__, id);
		return;
	}

	Mix_PlayChannel(-1, it->second.get(), 0);
}

static const std::unordered_map<std::string, TauntId> str_taunts{
	{"yes", TauntId::yes},
	{"no", TauntId::no},
	{"food", TauntId::food},
	{"wood", TauntId::wood},
	{"gold", TauntId::gold},
	{"stone", TauntId::stone},
	{"ehh", TauntId::ehh},
	{"nice try", TauntId::nice_try},
	{"fail", TauntId::nice_try},
	{"yay", TauntId::yay},
	{"in town", TauntId::in_town},
	{"owaah", TauntId::owaah},
	{"join", TauntId::join},
	{"join me", TauntId::join},
	{"come join", TauntId::join},
	{"disagree", TauntId::disagree},
	{"dont think so", TauntId::disagree},
	{"i dont think so", TauntId::disagree},
	{"start", TauntId::start},
	{"start pls", TauntId::start},
	{"pls start", TauntId::start},
	{"alpha", TauntId::alpha},
	{"whos the man", TauntId::alpha},
	{"attack", TauntId::attack},
	{"attack now", TauntId::attack},
	{"haha", TauntId::haha},
	{"mercy", TauntId::mercy},
	{"have mercy", TauntId::mercy},
	{"pls have mercy", TauntId::mercy},
	{"im weak", TauntId::mercy},
	{"hahaha", TauntId::hahaha},
	{"hahahaha", TauntId::hahaha},
	{"satisfaction", TauntId::satisfaction},
	{"satisfied", TauntId::satisfaction},
	{"nice", TauntId::nice},
	{"nice town", TauntId::nice},
	{"wtf", TauntId::wtf},
	{"get out", TauntId::get_out},
	{"go away", TauntId::get_out},
	{"let go", TauntId::let_go},
	{"yeah", TauntId::yeah},
	{"oh yeah", TauntId::yeah},
	{"wololo", TauntId::max},
	{"wololol", TauntId::max},
	{"wol", TauntId::max},
};

std::optional<TauntId> Audio::is_taunt(const std::string &s) {
	// filter s
	std::string filtered;

	// TODO make UTF8 friendly
	for (char v : s) {
		unsigned char ch = (unsigned char)(int)v;

		if (isalpha(ch) || isspace(ch))
			filtered += tolower(ch);
	}

	auto it = str_taunts.find(filtered);
	if (it != str_taunts.end())
		return it->second;

	// no exact match, see if it's a number
	int v;
	if (sscanf(s.data(), "%d", &v) == 1) {
		unsigned idx = (unsigned)std::abs(v);

		if (idx > 0 && idx - 1 < (unsigned)TauntId::max)
			return (TauntId)(idx - 1);
	}

	// TODO use levenshtein distance for typos

	// seems bogus, fail
	return std::nullopt;
}

void Audio::load_sfx(SfxId id, const std::vector<uint8_t> &data) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m_mix);

	std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)> sfx(Mix_LoadWAV_RW(SDL_RWFromConstMem(data.data(), data.size()), 1), Mix_FreeChunk);
	if (!sfx.get())
		throw std::runtime_error(std::string("cannot load audio ID ") + std::to_string((unsigned)id) + ": " + Mix_GetError());

	this->sfx.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple(sfx.release(), Mix_FreeChunk));
}

void Audio::play_sfx(SfxId id, int loops) {
	ZoneScoped;

	if (!enable_sound)
		return;

	// check if special sfxid
	switch (id) {
		case SfxId::bld_die_random:
			id = (SfxId)((unsigned)SfxId::bld_die1 + rand() % 3);
			break;
		case SfxId::villager_random:
			id = (SfxId)((unsigned)SfxId::villager1 + rand() % 7);
			break;
		case SfxId::villager_die_random:
			id = (SfxId)((unsigned)SfxId::villager_die1 + rand() % 10);
			break;
		case SfxId::villager_attack_random:
			id = (SfxId)((unsigned)SfxId::villager_attack1 + rand() % 3);
			break;
		case SfxId::priest_attack_random:
			id = (SfxId)((unsigned)SfxId::priest_attack1 + rand() % 2);
			break;
		case SfxId::invalid_select:
			id = SfxId::queue_error;
			break;
	}

	std::lock_guard<std::mutex> lk(m_mix);

	auto it = sfx.find(id);
	if (it == sfx.end()) {
		fprintf(stderr, "%s: cannot play sfx id %d: not found\n", __func__, id);
		return;
	}

	Mix_Chunk *chunk = it->second.get(); // Mix_PlayChannel used to be a macro, so just in case

	// TODO use priorities for sound effects.
	// prioritize: victory/defeat, UI sfx, combat sfx
	int ch = Mix_PlayChannel(-1, chunk, loops);
	if (ch == -1) {
		fprintf(stderr, "%s: failed to play sfx: %s\n", __func__, Mix_GetError());
		fprintf(stderr, "%s: currently playing: %d\n", __func__, Mix_Playing(-1));
	}
}

}
