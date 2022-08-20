#pragma once

#include <cstdint>

#include <string>
#include <vector>

namespace aoe {

static constexpr unsigned max_players = UINT8_MAX + 1u;

struct Resources final {
	int food, wood, gold, stone;

	Resources() : food(0), wood(0), gold(0), stone(0) {}
	Resources(int f, int w, int g, int s) : food(f), wood(w), gold(g), stone(s) {}
};

class PlayerSetting final {
public:
	std::string name;
	bool ai;
	int civ;
	unsigned index; // may overlap
	unsigned team;

	PlayerSetting() : name(), ai(false), civ(0), index(1), team(1) {}
};

class ScenarioSettings final {
public:
	std::vector<PlayerSetting> players;
	bool fixed_start;
	bool explored;
	bool all_technologies;
	bool cheating;
	bool square;
	bool restricted; // allow other players to also change settings
	unsigned width, height;
	unsigned popcap;
	unsigned age;
	unsigned seed;
	unsigned villagers;

	Resources res;

	ScenarioSettings();
};

}
