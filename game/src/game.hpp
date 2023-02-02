#pragma once

#include <cstdint>

#include <string>
#include <vector>
#include <map>

#include <mutex>
#include <set>

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
	PlayerSetting(const std::string &name, bool ai, int civ, unsigned team, Resources res) : name(name), ai(ai), civ(civ), team(team), res(res) {}
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

enum class EntityState {
	alive,
	dying,
	decaying,
};

enum class EntityType {
	town_center,
	barracks,
	bird1,
	villager,
};

static bool constexpr is_building(EntityType t) {
	return t >= EntityType::town_center && t <= EntityType::barracks;
}

class EntityView final {
public:
	IdPoolRef ref;
	EntityType type;

	unsigned color; // TODO /color/playerid/ ?
	float x, y, angle;

	// TODO add ui info
	unsigned subimage;
	EntityState state;

	EntityView() : ref(invalid_ref), type(EntityType::town_center), color(0), x(0), y(0), angle(0), subimage(0), state(EntityState::alive) {}
	EntityView(IdPoolRef ref, EntityType type, unsigned color, float x, float y, float angle=0) : ref(ref), type(type), color(color), x(x), y(y), angle(angle), subimage(0), state(EntityState::alive) {}
};

class Entity final {
public:
	IdPoolRef ref;
	EntityType type;

	unsigned color; // TODO /color/playerid/ ?
	float x, y, angle;

	// TODO add more params
	unsigned subimage;
	EntityState state;

	Entity(IdPoolRef ref) : ref(ref), type(EntityType::town_center), color(0), x(0), y(0), angle(0), subimage(0), state(EntityState::alive) {}

	Entity(IdPoolRef ref, EntityType type, unsigned color, float x, float y, float angle=0) : ref(ref), type(type), color(color), x(x), y(y), angle(angle), subimage(0), state(EntityState::alive) {}

	Entity(const EntityView &ev) : ref(ev.ref), type(ev.type), color(ev.color), x(ev.x), y(ev.y), angle(0), subimage(ev.subimage), state(ev.state) {}

	friend bool operator<(const Entity &lhs, const Entity &rhs) noexcept {
		return lhs.ref < rhs.ref;
	}

	void imgtick(unsigned n);
};

class PlayerAchievements final {
public:
	// too lazy to check if smaller types will fit, this should be fine.
	int64_t kills, losses, razings;
	size_t military_size;
	int64_t military_score;

	int64_t food, wood, stone, gold, tribute;
	size_t villager_count, unit_count;
	size_t explored_tiles, explored_max;
	int64_t economy_score;

	int64_t conversions, converted;
	int64_t temples, ruins, artifacts;
	int64_t religion_score;

	unsigned technologies;
	bool bronze_first, iron_first;
	int64_t technology_score;

	int64_t wonders;

	bool alive;
	int64_t score;

	int64_t recompute(size_t max_military, size_t max_villagers, unsigned max_tech) noexcept;
};

class PlayerView;

class Player final {
public:
	PlayerSetting init;
	Resources res;
	PlayerAchievements achievements;
	uint64_t explored_max;
	std::set<IdPoolRef> entities;
	bool alive;
	// TODO add team

	Player(const PlayerSetting&, size_t explored_max);

	PlayerView view() const noexcept;
};

class PlayerView final {
public:
	PlayerSetting init;
	Resources res;
	int64_t score;
	unsigned age;
	bool alive;

	PlayerView(const PlayerSetting &ps) : init(ps), res(ps.res), score(0), age(1), alive(true) {}
};

class GameView;

class Game final {
	std::mutex m;
	Terrain t;
	std::vector<PlayerView> players;
	// no IdPool as we have no control over IdPoolRefs: the server does
	std::set<Entity> entities;
	std::vector<EntityView> entities_killed;
	unsigned modflags, ticks, imgcnt;
	friend GameView;
public:
	Game();

	void tick(unsigned n);

	void resize(const ScenarioSettings &scn);
	void terrain_set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);

	void set_players(const std::vector<PlayerSetting>&);

	void player_died(unsigned);

	void entity_add(const EntityView &ev);
	bool entity_kill(IdPoolRef);
private:
	void imgtick(unsigned n);
};

class GameView final {
public:
	Terrain t;
	std::set<Entity> entities; // TODO use std::variant or Entity uniqueptr
	std::vector<EntityView> entities_killed;
	std::vector<PlayerView> players;
	std::vector<unsigned> players_died;

	GameView();

	bool try_read(Game&, bool reset=true);

	Entity *try_get(IdPoolRef) noexcept;
};

}
