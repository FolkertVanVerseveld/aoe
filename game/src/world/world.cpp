#include "../server.hpp"

#include "../legacy.hpp"

#include <chrono>

#include <tracy/Tracy.hpp>

namespace aoe {

void Server::tick() {
	ZoneScoped;
	// TODO stub
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

}
