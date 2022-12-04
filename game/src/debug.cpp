#include "debug.hpp"

#include <tracy/Tracy.hpp>

#include "engine.hpp"
#include "ui.hpp"

namespace aoe {

using namespace ui;

static ImU8 read_incoming(const ImU8 *ptr, size_t off) {
	std::deque<uint8_t> &data = *((std::deque<uint8_t>*)(void*)ptr);
	return data.at(off);
}

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
			std::string name;

			for (auto kv : s.peers) {
				const Peer &p = kv.first;

				//f.fmt("%3llu: %s:%s", i, p.host.c_str(), p.server.c_str());
				name = std::to_string(i) + ": " + p.host + ":" + p.server;

				if (ImGui::TreeNode(name.c_str())) {
					std::unique_lock<std::mutex> lkp(s.s.peer_ev_lock, std::defer_lock), lkd(s.s.data_lock, std::defer_lock);

					SOCKET sock = p.sock;
					ServerSocket &ss = s.s;

					std::lock(lkp, lkd);
					{
						size_t in = 0, out = 0;

						auto it = ss.data_in.find(sock);
						if (it != ss.data_in.end()) {
							in = it->second.size();
							f.fmt("incoming data: %zu", in);
							auto &data = it->second;
							mem_edit.ReadFn = read_incoming;
							mem_edit.DrawContents(&data, data.size());
						}

						it = ss.data_out.find(sock);
						if (it != ss.data_out.end())
							out = it->second.size();

						f.fmt("outcoming data: %zu", out);
					}
					lkd.unlock();

					{
						std::unique_lock<std::mutex> m_pending;

						size_t out = 0;

						auto it = ss.send_pending.find(sock);
						if (it != ss.send_pending.end())
							out = it->second.size();

						f.fmt("pending data: %zu", out);
					}

					lkp.unlock();

					ImGui::TreePop();
				}

				++i;
			}
		}

		f.fmt("Client running: %s", has_client ? "yes" : "no");
	}
}

}
