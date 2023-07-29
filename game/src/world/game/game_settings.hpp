#pragma once

#include <string>
#include <vector>
#include <map>

#include <idpool.hpp>

#include "../resources.hpp"

namespace aoe {

static constexpr unsigned max_players = UINT8_MAX + 1u;

class PlayerSetting final {
public:
	std::string name;
	int civ;
	unsigned team;
	Resources res;
	bool ai;

	PlayerSetting() : name(), civ(0), team(1), res(), ai(false) {}
	PlayerSetting(const std::string &name, int civ, unsigned team, Resources res) : name(name), civ(civ), team(team), res(res), ai(false) {}
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
	unsigned age;
	unsigned seed;
	unsigned villagers;

	Resources res;

	ScenarioSettings();

	void remove(IdPoolRef);
};

}
