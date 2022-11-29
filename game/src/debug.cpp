#include "debug.hpp"

#include <tracy/Tracy.hpp>

#include "engine.hpp"
#include "ui.hpp"

namespace aoe {

using namespace ui;

void Debug::show() {
	ZoneScoped;

	Frame f;
	Engine &e = *eng;

	if (!f.begin("Debug control"))
		return;

	int size = e.tp.size(), idle = e.tp.n_idle(), running = size - idle;
	f.fmt("Thread pool: %d threads, %d running", size, running);

	{
		std::lock_guard<std::mutex> lk(e.m);

		bool has_server = e.server.get() != nullptr;
		bool has_client = e.client.get() != nullptr;


		f.fmt("Server active: %s", has_server ? "yes" : "no");

		if (has_server) {
			Server &s = *e.server.get();

			f.fmt("running: %s", s.active() ? "yes" : "no");
			f.fmt("port: %u", s.port);
			f.fmt("protocol: %u", s.protocol);

			f.fmt("connected peers: %llu", (unsigned long long)s.peers.size());

			size_t i = 0;

			for (auto kv : s.peers) {
				const Peer &p = kv.first;

				f.fmt("%3llu: %s:%s", i, p.host.c_str(), p.server.c_str());

				++i;
			}
		}

		f.fmt("Client running: %s", has_client ? "yes" : "no");
	}
}

}
