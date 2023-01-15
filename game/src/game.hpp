#pragma once

#include <cstdint>

#include <string>
#include <vector>
#include <map>

#include <mutex>

#include "idpool.hpp"

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
	unsigned team;
	Resources res;

	PlayerSetting() : name(), ai(false), civ(0), team(1), res() {}
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
};

enum class TerrainTile {
	unknown, // either empty or unexplored
	desert,
	grass,
	water,
	deepwater,
	water_desert,
	grass_desert,
	desert_overlay,
	deepwater_overlay,
};

enum class TerrainType {
	islands,
	continents,
	normal,
	flat,
	bumpy,
	max,
};

// TODO introduce terrain block/chunk

class Terrain final {
	std::vector<uint8_t> tiles;
	std::vector<int8_t> hmap;
public:
	unsigned w, h, seed, players;
	bool wrap;

	Terrain();

	void resize(unsigned width, unsigned height, unsigned seed, unsigned players, bool wrap);

	void generate();

	uint8_t id_at(unsigned x, unsigned y);
	int8_t h_at(unsigned x, unsigned y);

	void fetch(std::vector<uint8_t> &tiles, std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned &w, unsigned &h);

	void set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);
};

enum class BuildingType {
	town_center,
	barracks,
};

class Building final {
public:
	IdPoolRef ref;
	BuildingType type;

	unsigned color;
	int x, y;

	Building(IdPoolRef ref, BuildingType type, unsigned color, int x, int y) : ref(ref), type(type), color(color), x(x), y(y) {}
};

class GameView;

class Game final {
	std::mutex m;
	Terrain t;
	friend GameView;
public:
	Game();

	void resize(const ScenarioSettings &scn);
	void terrain_set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);
};

class GameView final {
public:
	Terrain t;
	IdPool<Building> buildings; // TODO use std::variant or Entity uniqueptr

	GameView();

	bool try_read(Game&);
};

}
