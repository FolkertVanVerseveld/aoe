#include "game.hpp"
#include "server.hpp"

#include "legacy.hpp"

#include <chrono>

#include <tracy/Tracy.hpp>

namespace aoe {

void Server::tick() {
	ZoneScoped;
}

void Server::eventloop() {
	ZoneScoped;

	auto last = std::chrono::steady_clock::now();
	double dt = 0;

	while (m_running.load()) {
		// recompute as logic_gamespeed may change
		double interval_inv = logic_gamespeed * DEFAULT_TICKS_PER_SECOND;
		double interval = 1 / std::max(0.01, interval_inv);

		auto now = std::chrono::steady_clock::now();
		std::chrono::duration<double> elapsed = now - last;
		last = now;
		dt += elapsed.count();

		size_t steps = (size_t)(dt * interval_inv);

		// do steps
		for (; steps; --steps)
			tick();

		dt = fmod(dt, interval);

		unsigned us = 0;

		if (!steps)
			us = (unsigned)(interval * 1000 * 1000);
		// 100000000

		if (us > 500)
			std::this_thread::sleep_for(std::chrono::microseconds(us));
	}
}

Game::Game() : m(), t(), players() {}

void Game::resize(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m);

	t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
}

void Game::terrain_set(const std::vector<uint8_t> &tiles, const std::vector<int8_t> &hmap, unsigned x, unsigned y, unsigned w, unsigned h) {
	std::lock_guard<std::mutex> lk(m);
	t.set(tiles, hmap, x, y, w, h);
}

void Game::set_players(const std::vector<PlayerSetting> &lst) {
	std::lock_guard<std::mutex> lk(m);

	players.clear();

	for (const PlayerSetting &ps : lst)
		players.emplace_back(ps);
}

GameView::GameView() : t(), entities() {
	// TODO remove this test later on

#if 1
	for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
		entities.emplace(EntityType::town_center, i, 2, 1 + 3 * i);
		entities.emplace(EntityType::barracks, i, 2 + 3, 1 + 3 * i);
	}
#else
	unsigned i = 0;
	entities.emplace(EntityType::town_center, i, 2, 1 + 3 * i);
#endif
}

bool GameView::try_read(Game &g) {
	std::unique_lock lk(g.m, std::defer_lock);

	if (!lk.try_lock())
		return false;

	// TODO only copy what has changed
	t = g.t;
	players = g.players;

	return true;
}

}
