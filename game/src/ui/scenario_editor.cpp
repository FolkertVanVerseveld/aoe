#include "../ui.hpp"

#include "../engine.hpp"

#include "../string.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::show_editor_scenario() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *e->assets.get();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	if (!io.WantCaptureMouse) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		e->sdl->set_cursor(1);
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::MenuItem("Quit"))
			e->next_menu_state = MenuState::start;

		ImGui::EndMainMenuBar();
	}

	float menubar_bottom = vp->WorkPos.y;

	//50
	//625/768 -> 625-768=-143

	const gfx::ImageRef &bkg = a.at(io::DrsId::img_editor);

	float menubar_left = vp->WorkPos.x;

	// align center if menubar smaller than screen dimensions
	if (vp->WorkSize.x > bkg.bnds.w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - bkg.bnds.w) / 2;

	float t1, h = 50.0f;

	t1 = bkg.t0 + (bkg.t1 - bkg.t0) * h / bkg.bnds.h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_left + bkg.bnds.w, vp->WorkPos.y + h), ImVec2(bkg.s0, bkg.t0), ImVec2(bkg.s1, t1));

	h = 143.0f;
	float t0 = bkg.t0 + (bkg.t1 - bkg.t0) * (bkg.bnds.h - h) / bkg.bnds.h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y + vp->WorkSize.y - h), ImVec2(menubar_left + bkg.bnds.w, vp->WorkPos.y + vp->WorkSize.y), ImVec2(bkg.s0, t0), ImVec2(bkg.s1, bkg.t1));

	fd.Display();

	if (fd.HasSelected()) {
		std::string path(fd.GetSelected().string());
		try {
			scn.load(path.c_str());
		} catch (std::runtime_error &e) {
			fprintf(stderr, "%s: cannot load scn: %s", __func__, e.what());
		}
		fd.ClearSelected();
	}

	if (!scn.data.empty()) {
		FontGuard fg(NULL);
		mem.DrawWindow("raw filedata", scn.data.data(), scn.data.size());

		Frame f;

		if (f.begin("scenario info")) {
			bool printable = true;

			for (unsigned i = 0; i < sizeof(scn.version); ++i) {
				if (!isprint((unsigned char)scn.version[i])) {
					printable = false;
					break;
				}
			}

			if (printable) {
				std::string buf;
				cstrncpy(buf, scn.version, sizeof(scn.version));
				f.fmt("version: %s", buf.c_str());
			}

			f.fmt("length: %u", scn.length);

			f.fmt("players: %u", scn.players);

			f.str("Instructions:");
			ImGui::TextWrapped("%s", scn.instructions.c_str());
		}
	}

	Frame f;

	if (!f.begin("scenario control"))
		return;

	if (f.btn("load map")) {
		fd.Open();
	}
}

}

}
