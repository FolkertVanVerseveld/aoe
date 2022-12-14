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

void Debug::show(bool &open) {
	ZoneScoped;

	if (!open)
		return;

	Frame f;
	Engine &e = *eng;

	if (!f.begin("Debug control", open))
		return;

	int displays = SDL_GetNumVideoDisplays();
	f.fmt("Display count: %d", displays);

	for (int i = 0; i < displays; ++i) {
		std::string lbl(std::string("Display ") + std::to_string(i));

		if (ImGui::TreeNode(lbl.c_str())) {
			SDL_Rect bnds{ 0 }, bnds2{ 0 };

			SDL_GetDisplayBounds(i, &bnds);
			f.fmt("%4dx%4d at %4d,%4d", bnds.w, bnds.h, bnds.x, bnds.y);

			SDL_GetDisplayUsableBounds(i, &bnds2);
			f.fmt("%4dx%4d at %4d,%4d usable", bnds2.w, bnds2.h, bnds2.x, bnds2.y);

			SDL_DisplayMode mode;
			SDL_GetCurrentDisplayMode(i, &mode);
			f.fmt("%4dx%4d @%dHz format %s (%08X)", mode.w, mode.h, mode.refresh_rate, SDL_GetPixelFormatName(mode.format), mode.format);

			double aspect = (double)bnds.w / bnds.h;

			// try to determine exact ratio
			long long llw = (long long)bnds.w, llh = (long long)bnds.h;
			char *fmt = "unknown format";

			// check both ways as we may have rounding errors.
			// i didn't verify this formula, so there might be situations were it isn't exact
			if (llw / 4 * 3 == llh && llh / 3 * 4 == llw)
				fmt = "4:3";
			else if (llw / 16 * 9 == llh && llh / 9 * 16 == llw)
				fmt = "16:9";
			else if (llw / 21 * 9 == llh && llh / 9 * 21 == llw)
				fmt = "21:9";

			f.fmt("aspect ratio: %s (%.2f)\n", fmt, aspect);

			int modes = SDL_GetNumDisplayModes(i);
			f.fmt("modes: %d", modes);

			ImGui::TreePop();
		}
	}

	SDL_DisplayMode mode;
	SDL_GetWindowDisplayMode(e.sdl->window, &mode);

	f.fmt("using display %d", SDL_GetWindowDisplayIndex(e.sdl->window));
	f.fmt("%4dx%4d @%dHz format %s (%08X)", mode.w, mode.h, mode.refresh_rate, SDL_GetPixelFormatName(mode.format), mode.format);

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
				ClientInfo &ci = kv.second;

				//f.fmt("%3llu: %s:%s", i, p.host.c_str(), p.server.c_str());
				name = std::to_string(i) + ": " + p.host + ":" + p.server;

				if (!ci.username.empty())
					name += " " + ci.username;

				if (ImGui::TreeNode(name.c_str())) {
#if 0
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
#else
					IdPoolRef ref = ci.ref;

					f.fmt("ref (%u,%u)", ref.first, ref.second);

					unsigned flags = ci.flags;
					bool ready = !!(flags & (unsigned)ClientInfoFlags::ready);

					f.chkbox("ready", ready);

					if (ready)
						flags |= (unsigned)ClientInfoFlags::ready;
					else
						flags &= ~(unsigned)ClientInfoFlags::ready;

					ci.flags = flags;
#endif

					ImGui::TreePop();
				}

				++i;
			}
		}

		f.fmt("Client running: %s", has_client ? "yes" : "no");

		if (has_client) {
			Client &c = *e.client.get();

			f.fmt("connected: %s", c.m_connected ? "yes" : "no");
			f.fmt("host: %s", c.host.c_str());
			f.fmt("connected peers: %llu", (unsigned long long)c.peers.size());

			size_t i = 0;

			for (auto kv : c.peers) {
				ClientInfo &ci = kv.second;

				if (ImGui::TreeNode(ci.username.c_str())) {
					IdPoolRef ref = ci.ref;

					f.fmt("ref (%u,%u)", ref.first, ref.second);

					ImGui::TreePop();
				}

				++i;
			}
		}
	}
}

}
