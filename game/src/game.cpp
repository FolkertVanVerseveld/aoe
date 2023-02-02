#include "game.hpp"
#include "server.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

enum class GameMod {
	terrain = 1 << 0,
	entities = 1 << 1,
	players = 1 << 2,
};

Game::Game() : m(), t(), players(), entities(), entities_killed(), modflags((unsigned)-1), ticks(0), imgcnt(0) {}

void Game::resize(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m);
	t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	modflags |= (unsigned)GameMod::terrain;
}

void Game::tick(unsigned n) {
	std::lock_guard<std::mutex> lk(m);
	ticks += n;

	imgcnt += n;

	if (imgcnt / 5) {
		imgtick(imgcnt / 5);
		imgcnt /= 5;
	}
}

void Game::imgtick(unsigned n) {
	for (const Entity &e : entities) {
		Entity &ent = const_cast<Entity&>(e);
		ent.imgtick(n);
	}

	modflags |= (unsigned)GameMod::entities;
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

	modflags |= (unsigned)GameMod::players;
}

void Game::player_died(unsigned pos) {
	std::lock_guard<std::mutex> lk(m);

	players.at(pos).alive = false;
	modflags |= (unsigned)GameMod::players;
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
	players_died.clear();

	std::vector<bool> players_alive;
	size_t size = players.size();

	for (PlayerView &pv : players)
		players_alive.emplace_back(pv.alive);

	players = g.players;

	for (unsigned i = 0; i < std::min(size, g.players.size()); ++i)
		if (players_alive[i] != g.players[i].alive)
			players_died.emplace_back(i);
	// end todo

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
