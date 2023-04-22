#include "game.hpp"
#include "server.hpp"

#include "../engine.hpp"
#include "../net/protocol.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

enum class GameMod {
	terrain = 1 << 0,
	entities = 1 << 1,
	players = 1 << 2,
};

Game::Game() : m(), t(), players(), entities(), entities_killed(), modflags((unsigned)-1), ticks(0), running(false) {}

void Game::resize(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m);
	t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	modflags |= (unsigned)GameMod::terrain;
}

void Game::tick(unsigned n) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(m);
	running = true;
	ticks += n;
	imgtick(n);
}

void Game::imgtick(unsigned n) {
	ZoneScoped;
	for (const Entity &e : entities) {
		Entity &ent = const_cast<Entity&>(e);
		if (!ent.imgtick(n)) {
			std::optional<SfxId> sfx(ent.sfxtick());
			if (sfx.has_value()) {
				EngineView ev;
				ev.play_sfx(sfx.value());
			}
		}
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

void Game::set_player_score(unsigned idx, const NetPlayerScore &ps) {
	std::lock_guard<std::mutex> lk(m);

	if (idx < players.size()) {
		PlayerView &p = players[idx];
		p.score = ps.score;
		p.alive = ps.alive;
	}

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

void Game::entity_update(const EntityView &ev) {
	EntityView v(ev);
	std::lock_guard<std::mutex> lk(m);

	bool statechange = false;
	auto it = entities.find(v);
	if (it != entities.end()) {
		// TODO subimage is always zero. prob forgot to update it somewhere on the server side or forgot to send it to the clients.
		// only update subimage if it's state has been changed
		// e.g. when a unit goes from alive to dying, we need to reset the animation sequence
		if (it->state == ev.state) {
			v.subimage = it->subimage;
			v.xflip = it->xflip;
		} else {
			statechange = true;
		}

		entities.erase(it);
	}

	switch (ev.state) {
	case EntityState::dying:
		if (statechange)
			entities_killed.emplace_back(v);
		break;
	}

	auto p = entities.emplace(v);
	// if the entity's state changed, its subimage is zero, which is usually wrong. force recomputing fixes that
	const_cast<Entity&>(*p.first).imgtick(0);

	modflags |= (unsigned)GameMod::entities;
}

bool Game::entity_kill(IdPoolRef ref) {
	std::lock_guard<std::mutex> lk(m);

	auto it = entities.find(ref);
	if (it == entities.end())
		return false;

	entities_killed.emplace_back(*it);
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
