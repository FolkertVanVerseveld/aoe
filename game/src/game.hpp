#pragma once

#include <cstdint>

#include <atomic>
#include <string>
#include <vector>
#include <map>

#include <mutex>
#include <set>

#include "../engine/audio.hpp"

#include <idpool.hpp>

#include "world/entity_info.hpp"

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
	attack,
	attack_follow,
	moving,
};

enum class EntityTaskType {
	move,
	infer, // use context to determine task
	attack,
	train_unit,
};

static bool constexpr is_building(EntityType t) {
	return t >= EntityType::town_center && t <= EntityType::barracks;
}

static bool constexpr is_resource(EntityType t) {
	return t >= EntityType::berries && t <= EntityType::dead_tree2;
}

static bool constexpr is_worker(EntityType t) {
	return t >= EntityType::villager && t <= EntityType::worker_wood2;
}

class Entity;

class EntityView final {
public:
	IdPoolRef ref;
	EntityType type; // TODO remove this and use stats.type

	unsigned playerid;
	float x, y, angle;

	// TODO add ui info
	unsigned subimage;
	EntityState state;
	bool xflip;

	EntityStats stats;

	EntityView();
	EntityView(const Entity&);
};

class EntityTask final {
public:
	EntityTaskType type;
	IdPoolRef ref1, ref2;
	uint32_t x, y;
	unsigned info_type, info_value;

	// TODO use floating point number x,y
	// move to area
	EntityTask(IdPoolRef ref, uint32_t x, uint32_t y);
	EntityTask(EntityTaskType type, IdPoolRef ref1, IdPoolRef ref2);
	// train unit
	EntityTask(IdPoolRef ref, EntityType t);
};

class WorldView;

enum class ParticleType {
	moveto,
};

// TODO stub
class Particle final {
public:
	IdPoolRef ref;
	ParticleType type;

	float x, y;

	unsigned subimage;

	Particle(IdPoolRef ref);
	Particle(IdPoolRef ref, ParticleType type, float x, float y);

	/* Animate and return whether there's more images. Returning false means the animation has ended and the particle shall be destroyed. */
	bool imgtick(unsigned n) noexcept;

	friend bool operator<(const Particle &lhs, const Particle &rhs) noexcept {
		return lhs.ref < rhs.ref;
	}
};

class Entity final {
public:
	IdPoolRef ref;
	EntityType type;

	unsigned playerid;
	float x, y, angle;

	IdPoolRef target_ref; // if == invalid_ref, use target_x,target_y
	float target_x, target_y;

	// TODO add more params
	unsigned subimage;
	EntityState state;
	bool xflip;

	EntityStats stats;

	Entity(IdPoolRef ref);
	Entity(IdPoolRef ref, EntityType type, unsigned playerid, float x, float y, float angle=0, EntityState state=EntityState::alive);

	Entity(const EntityView &ev);

	friend bool operator<(const Entity &lhs, const Entity &rhs) noexcept {
		return lhs.ref < rhs.ref;
	}

	bool die() noexcept;
	void decay() noexcept;

	bool tick(WorldView&) noexcept;
	bool hit(WorldView&, Entity &aggressor) noexcept;

	constexpr bool is_alive() const noexcept {
		return state != EntityState::dying && state != EntityState::decaying;
	}

	bool task_move(float x, float y) noexcept;
	bool task_attack(Entity&) noexcept;

	std::optional<SfxId> sfxtick() noexcept;

	bool imgtick(unsigned n) noexcept;
private:
	void reset_anim() noexcept;
	bool set_state(EntityState) noexcept;
	bool try_state(EntityState) noexcept;
	bool move() noexcept;
	bool attack(WorldView&) noexcept;
	bool in_range(const Entity&) const noexcept;

	/* Compute facing angle and return euclidean distance. */
	float lookat(float x, float y) noexcept;
};

class PlayerAchievements final {
public:
	// too lazy to check if smaller types will fit, this should be fine.
	uint32_t kills, losses, razings;
	size_t military_size;
	uint32_t military_score;

	uint64_t food, wood, stone, gold;
	int64_t tribute;
	size_t villager_count, unit_count;
	size_t explored_tiles, explored_max;
	int32_t economy_score;

	uint32_t conversions, converted;
	uint32_t temples, ruins, artifacts;
	uint32_t religion_score;

	unsigned technologies;
	bool most_technologies;
	bool bronze_first, iron_first;
	uint32_t technology_score;

	unsigned wonders;
	unsigned char age;

	bool alive;
	int64_t score;

	int64_t recompute() noexcept;
};

class PlayerView;

class Player final {
public:
	PlayerSetting init;
	Resources res;
private:
	PlayerAchievements achievements;
public:
	uint64_t explored_max;
	std::set<IdPoolRef> entities;
	bool alive;
	// TODO add team

	Player(const PlayerSetting&, size_t explored_max);

	PlayerView view() const noexcept;

	PlayerAchievements get_score() noexcept;

	void killed_unit();
	void killed_building();
	void lost_entity(IdPoolRef);
};

class PlayerView final {
public:
	PlayerSetting init;
	Resources res;
	uint32_t military;
	int64_t score;
	unsigned age;
	bool alive;

	PlayerView(const PlayerSetting &ps) : init(ps), res(ps.res), military(0), score(0), age(1), alive(true) {}
};

class GameView;
struct NetPlayerScore;

/**
 * Client side game state. Some vars are duplicated from World,
 * but may be slightly altered as the client has little control over their internal state.
 */
class Game final {
	std::mutex m;
	Terrain t;
	std::vector<PlayerView> players;
	// no IdPool as we have no control over IdPoolRefs: the server does
	std::set<Entity> entities;
	std::vector<EntityView> entities_killed;
	unsigned modflags, ticks;
	unsigned team_won;
	friend GameView;
public:
	std::atomic<bool> running;

	Game();

	void tick(unsigned n);

	void resize(const ScenarioSettings &scn);
	void terrain_set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);

	void set_players(const std::vector<PlayerSetting>&);
	void set_player_score(unsigned idx, const NetPlayerScore&);

	void player_died(unsigned);

	void entity_add(const EntityView &ev);
	bool entity_kill(IdPoolRef);
	void entity_update(const EntityView &ev);

	void gameover(unsigned team) noexcept;
	unsigned winning_team() noexcept;

	PlayerView pv(unsigned);
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
