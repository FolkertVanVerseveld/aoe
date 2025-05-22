#include "audio.hpp"

#include <cctype>
#include <cmath>

#include <array>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../debug.hpp"

#define SAMPLING_FREQ 48000
#define NUM_CHANNELS 2

namespace aoe {

Audio::Audio() : freq(0), channels(0), format(0), music(nullptr, Mix_FreeMusic), music_mute(false), sfx_mute(false), music_file()
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
}

Audio::~Audio() {
	music.reset();
	Mix_Quit();
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

	if (!music_mute)
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

void Audio::mute_music() {
	if (!music_mute)
		stop_music();

	music_mute = true;
}

void Audio::unmute_music() {
	if (!music_mute)
		return;

	music_mute = false;
	// restart if music is loaded
	if (music.get())
		Mix_PlayMusic(music.get(), 0);
}

void Audio::mute_sfx() {
	if (sfx_mute)
		return;

	Mix_HaltChannel(-1);
	sfx_mute = true;
}

void Audio::unmute_sfx() {
	sfx_mute = false;
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

	if (sfx_mute)
		return;

	std::lock_guard<std::mutex> lk(m_mix);

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

	auto it = sfx.find(id);
	if (it == sfx.end()) {
		fprintf(stderr, "%s: cannot play sfx id %d: not found\n", __func__, id);
		return;
	}

	Mix_Chunk *chunk = it->second.get(); // Mix_PlayChannel used to be a macro, so just in case
	Mix_PlayChannel(-1, chunk, loops);
}

}
