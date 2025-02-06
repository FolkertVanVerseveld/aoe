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

static constexpr unsigned max_legacy_players = 8 + 1; // gaia

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

extern ScenarioSettings sp_scn;

}
