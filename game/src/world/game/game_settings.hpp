#pragma once

#include <array>
#include <string>
#include <vector>
#include <map>

#include <idpool.hpp>

#include "../resources.hpp"
#include "../terrain.hpp"

namespace aoe {

static constexpr unsigned max_players = UINT8_MAX + 1u;

static constexpr unsigned first_player_idx = 1, first_team_idx = 1;

static constexpr unsigned max_legacy_players = 8 + first_player_idx; // gaia

static constexpr unsigned min_map_size = 8, max_map_size = 250;
static constexpr unsigned min_popcap = 5, max_popcap = 500;
static constexpr unsigned min_villagers = 1, max_villagers = 20;

class PlayerSetting final {
public:
	std::string name;
	unsigned color; // player
	int civ;
	unsigned team;
	Resources res;
	bool active;
	bool ai;

	PlayerSetting() : name(), color(0), civ(0), team(1), res(), active(false), ai(false) {}
	PlayerSetting(const std::string &name, int civ, unsigned team, Resources res) : name(name), color(0), civ(civ), team(team), res(res), active(false), ai(false) {}
	PlayerSetting(const PlayerSetting&) = default;
};

class ScenarioSettings final {
public:
	std::vector<PlayerSetting> players;
	std::map<IdPoolRef, unsigned> owners; // refs to player setting
	bool fixed_start;
	bool explored;
	bool all_technologies;
	bool cheating;
	bool square;
	bool wrap;
	bool restricted; // allow other players to also change settings
	bool reorder; // allow to move players up and down in the list
	unsigned width, height;
	unsigned popcap;
	int age;
	unsigned seed;
	unsigned villagers;
	TerrainType type;

	Resources res;

	ScenarioSettings();

	void remove(IdPoolRef);
};

extern unsigned sp_player_count, sp_player_ui_count;
extern std::array<PlayerSetting, max_legacy_players> sp_players;

void sp_game_settings_randomize();

extern ScenarioSettings sp_scn;

}
