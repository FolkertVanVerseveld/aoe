#include "debug.hpp"

#include "engine.hpp"
#include "ui.hpp"

#include "engine/gfx.hpp"

namespace aoe {

using namespace ui;

static ImU8 read_incoming(const ImU8 *ptr, size_t off) {
	std::deque<uint8_t> &data = *((std::deque<uint8_t>*)(void*)ptr);
	return data.at(off);
}

void Debug::show_texture_map() {
	ZoneScoped;

	if (!show_tm)
		return;

	Frame f;

	if (!f.begin("Texturemap", show_tm, ImGuiWindowFlags_HorizontalScrollbar))
		return;

	Engine &e = *eng;
	Assets &a = *e.assets.get();
	ImTextureID tex = (ImTextureID)1;
	ImGui::Image(tex, ImVec2(a.ts_ui.w, a.ts_ui.h));
}

void Debug::show_display_info(Frame &f) {
	ZoneScoped;

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
			const char *fmt = "unknown format";

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

	Engine &e = *eng;
	SDL_DisplayMode mode;
	SDL_GetWindowDisplayMode(e.sdl->window, &mode);

	f.fmt("using display %d", SDL_GetWindowDisplayIndex(e.sdl->window));
	f.fmt("%4dx%4d @%dHz format %s (%08X)", mode.w, mode.h, mode.refresh_rate, SDL_GetPixelFormatName(mode.format), mode.format);
}

void Debug::show(bool &open) {
	ZoneScoped;

	if (!open)
		return;

	Frame f;
	Engine &e = *eng;

	if (!f.begin("Debug control", open))
		return;

	ImGuiIO &io = ImGui::GetIO();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	show_display_info(f);

	int size = e.tp.size(), idle = e.tp.n_idle(), running = size - idle;
	f.fmt("Thread pool: %d threads, %d running", size, running);

	{
		std::lock_guard<std::mutex> lk(e.m);

		bool has_server = e.server.get() != nullptr;
		bool has_client = e.client.get() != nullptr;


		f.fmt("Server active: %s", has_server ? "yes" : "no");

		if (has_server) {
			IServer *is = e.server.get();

			f.fmt("running: %s", is->active() ? "yes" : "no");

			Server *s = dynamic_cast<Server*>(is);
			if (s) {
				f.fmt("port: %u", s->port);
				f.fmt("protocol: %u", s->protocol);

				f.fmt("connected peers: %llu", (unsigned long long)s->peers.size());

				size_t i = 0;
				std::string name;

				for (auto kv : s->peers) {
					const Peer &p = kv.first;
					ClientInfo &ci = kv.second;

					//f.fmt("%3llu: %s:%s", i, p.host.c_str(), p.server.c_str());
					name = std::to_string(i) + ": " + p.host + ":" + p.server;

					if (!ci.username.empty())
						name += " " + ci.username;

					if (ImGui::TreeNode(name.c_str())) {
						IdPoolRef ref = ci.ref;

						f.fmt("ref (%u,%u)", ref.first, ref.second);

						unsigned flags = ci.flags;
						ci_ready = !!(flags & (unsigned)ClientInfoFlags::ready);

						f.chkbox("ready", ci_ready);

						if (ci_ready)
							flags |= (unsigned)ClientInfoFlags::ready;
						else
							flags &= ~(unsigned)ClientInfoFlags::ready;

						ci.flags = flags;

						ImGui::TreePop();
					}

					++i;
				}
			}
		}

		f.fmt("Client running: %s", has_client ? "yes" : "no");

		if (has_client) {
			IClient *ic = e.client.get();

			f.fmt("connected: %s", ic->connected()  ? "yes" : "no");

			Client *c = dynamic_cast<Client*>(ic);
			if (c) {
				f.fmt("host: %s", c->host.c_str());
				f.fmt("ref: (%u,%u)", c->me.first, c->me.second);
				f.fmt("connected peers: %llu", (unsigned long long)c->peers.size());

				size_t i = 0;

				for (auto kv : c->peers) {
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

	f.chkbox("Show Texture Map", show_tm);

	show_texture_map();
}

#if TRACY_ENABLE
ImageCapture::ImageCapture(GLsizei w, GLsizei h) : m_fiFence{0}, m_fiIdx(0), m_fiQueue(), w(w), h(h) {
	glGenTextures(4, m_fiTexture);
	glGenFramebuffers(4, m_fiFramebuffer);
	glGenBuffers(4, m_fiPbo);

	for (int i = 0; i < 4; ++i) {
		glBindTexture(GL_TEXTURE_2D, m_fiTexture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		glBindFramebuffer(GL_FRAMEBUFFER, m_fiFramebuffer[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_fiTexture[i], 0);

		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_fiPbo[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, w * h * 4, nullptr, GL_STREAM_READ);
	}

	GLCHK;
}

void ImageCapture::step(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1) {
	while (!m_fiQueue.empty()) {
		const auto fiIdx = m_fiQueue.front();

		if (glClientWaitSync(m_fiFence[fiIdx], 0, 0) == GL_TIMEOUT_EXPIRED)
			break;

		glDeleteSync(m_fiFence[fiIdx]);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, m_fiPbo[fiIdx]);
		auto ptr = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, w * h * 4, GL_MAP_READ_BIT);
		FrameImage(ptr, w, h, m_fiQueue.size(), true);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
		m_fiQueue.erase(m_fiQueue.begin());
	}

	assert(m_fiQueue.empty() || m_fiQueue.front() != m_fiIdx); // check for buffer overflow
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fiFramebuffer[m_fiIdx]);
	glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fiFramebuffer[m_fiIdx]);
	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	m_fiFence[m_fiIdx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	m_fiQueue.emplace_back(m_fiIdx);
	m_fiIdx = (m_fiIdx + 1) % 4;
}
#endif

}
