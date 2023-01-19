#include "../server.hpp"

#include "../legacy.hpp"

#include <chrono>

#include <tracy/Tracy.hpp>

namespace aoe {

World::World() : m(), t(), entities(), players(), scn(), logic_gamespeed(1.0) {}

void World::load_scn(const ScenarioSettings &scn) {
	ZoneScoped;

	// TODO filter bogus settings
	this->scn.width = scn.width;
	this->scn.height = scn.height;
	this->scn.popcap = scn.popcap;
	this->scn.age = scn.age;
	this->scn.seed = scn.seed;
	this->scn.villagers = scn.villagers;

	this->scn.res = scn.res;

	this->scn.fixed_start = scn.fixed_start;
	this->scn.explored = scn.explored;
	this->scn.all_technologies = scn.all_technologies;
	this->scn.cheating = scn.cheating;
	this->scn.square = scn.square;
	this->scn.wrap = scn.wrap;

	t.resize(this->scn.width, this->scn.height, this->scn.seed, this->scn.players.size(), this->scn.wrap);
}

void World::create_terrain() {
	ZoneScoped;
	this->t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	this->t.generate();
}

NetTerrainMod World::fetch_terrain(int x, int y, unsigned &w, unsigned &h) {
	ZoneScoped;
	assert(x >= 0 && y >= 0);

	NetTerrainMod tm;
	t.fetch(tm.tiles, tm.hmap, 0, 0, w, h);

	tm.x = x; tm.y = y; tm.w = w; tm.h = h;

	return tm;
}

void World::tick() {
	ZoneScoped;
	// TODO stub
}

void World::eventloop(Server &s) {
	ZoneScoped;

	auto last = std::chrono::steady_clock::now();
	double dt = 0;

	while (s.m_running.load()) {
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

}
