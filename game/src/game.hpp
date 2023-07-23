#pragma once

#include <cstdint>

#include <atomic>
#include <string>
#include <vector>
#include <optional>
#include <map>

#include <mutex>
#include <set>

#include "world/terrain.hpp"

#include <idpool.hpp>

#include "world/entity_info.hpp"
#include "world/particle.hpp"
#include "world/resources.hpp"

#include "world/game/game_settings.hpp"
#include "world/entity.hpp"

namespace aoe {

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
	std::set<IdPoolRef> entities_spawned;
	std::vector<EntityView> entities_killed;
	// no IdPool as we have no control over IdPoolRefs: the server does
	std::set<Particle> particles;
	std::set<IdPoolRef> particles_spawned;
	unsigned modflags, ticks;
	unsigned team_won;
	friend GameView;
public:
	std::atomic<bool> running;

	Game();

	void tick(unsigned n);

	void resize(const ScenarioSettings &scn);
	void terrain_set(const std::vector<uint16_t> &tiles, const std::vector<uint8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h);

	void set_players(const std::vector<PlayerSetting>&);
	void set_player_score(unsigned idx, const NetPlayerScore&);
	void set_player_resources(unsigned idx, const Resources &res);

	void player_died(unsigned);

	void entity_add(const EntityView &ev);
	void entity_spawn(const EntityView &ev);
	bool entity_kill(IdPoolRef);
	void entity_update(const EntityView &ev);

	void entities_set(std::set<Entity> &&ent);

	void particle_spawn(const Particle &p);

	void gameover(unsigned team) noexcept;
	unsigned winning_team() noexcept;

	std::optional<PlayerView> try_pv(unsigned);
	PlayerView pv(unsigned);
private:
	void imgtick(unsigned n);
};

class GameView final {
public:
	Terrain t;
	std::set<Entity> entities;
	std::set<IdPoolRef> entities_spawned;
	std::vector<EntityView> entities_killed;
	std::set<Particle> particles;
	std::set<IdPoolRef> particles_spawned;
	std::vector<PlayerView> players;
	std::vector<unsigned> players_died;

	GameView();

	bool try_read(Game&, bool reset=true);

	Entity *try_get(IdPoolRef) noexcept;
	Entity &get(IdPoolRef);
};

}
