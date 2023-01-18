#include "game.hpp"
#include "server.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

enum class GameMod {
	terrain = 1 << 0,
	entities = 1 << 1,
};

Game::Game() : m(), t(), players(), entities(), entities_killed(), modflags((unsigned)-1) {}

void Game::resize(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m);
	t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	modflags |= (unsigned)GameMod::terrain;
}

void Game::terrain_set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h) {
	std::lock_guard<std::mutex> lk(m);

	t.set(tiles, hmap, x, y, w, h);
	modflags |= (unsigned)GameMod::terrain;
}

void Game::set_players(const std::vector<PlayerSetting> &lst) {
	std::lock_guard<std::mutex> lk(m);

	players.clear();

	for (const PlayerSetting &ps : lst)
		players.emplace_back(ps);
}

void Game::entity_add(const EntityView &ev) {
	std::lock_guard<std::mutex> lk(m);

	entities.emplace(ev);
	modflags |= (unsigned)GameMod::entities;
}

bool Game::entity_kill(IdPoolRef ref) {
	std::lock_guard<std::mutex> lk(m);

	auto it = entities.find(ref);
	if (it == entities.end())
		return false;

	entities_killed.emplace_back(it->ref, it->type, it->color, it->x, it->y);
	entities.erase(it);

	modflags |= (unsigned)GameMod::entities;

	return true;
}

GameView::GameView() : t(), entities(), entities_killed() {}

bool GameView::try_read(Game &g, bool reset) {
	std::unique_lock lk(g.m, std::defer_lock);

	if (!lk.try_lock())
		return false;

	if (g.modflags & (unsigned)GameMod::terrain)
		t = g.t;

	// TODO only copy what has changed
	players = g.players;

	if (g.modflags & (unsigned)GameMod::entities) {
		entities = g.entities;
		entities_killed = g.entities_killed;

		if (reset)
			g.entities_killed.clear();
	}

	if (reset)
		g.modflags = 0;

	return true;
}

Entity *GameView::try_get(IdPoolRef ref) noexcept {
	auto it = entities.find(ref);
	return it == entities.end() ? nullptr : (Entity*)&*it;
}

}
