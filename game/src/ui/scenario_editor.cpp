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

	float menubar_bottom = vp->WorkPos.y;

	//50
	//625/768 -> 625-768=-143

	const gfx::ImageRef &bkg = a.at(io::DrsId::img_editor);

	float menubar_left = vp->WorkPos.x;
	float menubar_top = vp->WorkPos.y;

	// align center if menubar smaller than screen dimensions
	if (vp->WorkSize.x > bkg.bnds.w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - bkg.bnds.w) / 2;

	float t1, h = 50.0f;

	t1 = bkg.t0 + (bkg.t1 - bkg.t0) * h / bkg.bnds.h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_left + bkg.bnds.w, vp->WorkPos.y + h), ImVec2(bkg.s0, bkg.t0), ImVec2(bkg.s1, t1));

	h = 143.0f;
	float t0 = bkg.t0 + (bkg.t1 - bkg.t0) * (bkg.bnds.h - h) / bkg.bnds.h;

	float menubar2_top = vp->WorkPos.y + vp->WorkSize.y - h;
	float menubar2_bottom = vp->WorkPos.y + vp->WorkSize.y;

	lst->AddImage(e->tex1, ImVec2(menubar_left, menubar2_top), ImVec2(menubar_left + bkg.bnds.w, menubar2_top + h), ImVec2(bkg.s0, t0), ImVec2(bkg.s1, bkg.t1));

	// draw buttons
	BackgroundColors col;

	col = a.bkg_cols.at(io::DrsId::bkg_editor_menu);

	const float padding = 2;
	// XXX times scale or not?
	float btn_x = menubar_left + padding * scale;
	float btn_y = menubar_top + padding * scale;
	float menubar_w = bkg.bnds.w;

	float btn_w = 110, btn_h = 22;

	float menubar_right = std::min(io.DisplaySize.x, menubar_left + menubar_w);

	if (frame_btn(col, "Map", btn_x, btn_y, btn_w, btn_h, scale, scn_edit.top_btn_idx == 0)) {
		scn_edit.top_btn_idx = 0;
	}

	if (frame_btn(col, "Terrain", btn_x + (btn_w + padding) * 1 * scale, btn_y, btn_w, btn_h, scale, scn_edit.top_btn_idx == 1)) {
		scn_edit.top_btn_idx = 1;
	}

	if (frame_btn(col, "Players", btn_x + (btn_w + padding) * 2 * scale, btn_y, btn_w, btn_h, scale, scn_edit.top_btn_idx == 2)) {
		scn_edit.top_btn_idx = 2;
	}

	if (frame_btn(col, "Units", btn_x + (btn_w + padding) * 3 * scale, btn_y, btn_w, btn_h, scale)) {
		;
	}

	if (frame_btn(col, "Diplomacy", btn_x + (btn_w + padding) * 4 * scale, btn_y, btn_w, btn_h, scale)) {
		;
	}

	if (frame_btn(col, "Individual Victory", btn_x, btn_y + (btn_h + padding) * scale, btn_w, btn_h, scale)) {
		;
	}

	if (frame_btn(col, "Global Victory", btn_x + (btn_w + padding) * 1 * scale, btn_y + (btn_h + padding) * scale, btn_w, btn_h, scale)) {
		;
	}

	if (frame_btn(col, "Options", btn_x + (btn_w + padding) * 2 * scale, btn_y + (btn_h + padding) * scale, btn_w, btn_h, scale)) {
		;
	}

	if (frame_btn(col, "Messages", btn_x + (btn_w + padding) * 3 * scale, btn_y + (btn_h + padding) * scale, btn_w, btn_h, scale)) {
		;
	}

	if (frame_btn(col, "Cinematics", btn_x + (btn_w + padding) * 4 * scale, btn_y + (btn_h + padding) * scale, btn_w, btn_h, scale)) {
		;
	}

	// 11, 636 - 625
	str2(ImVec2(menubar_left + 11 * scale, menubar2_top + 11 * scale), "Map");

	// 182, 725 - 625

	if (frame_btn(col, "Generate Map", menubar_left + 182 * scale, menubar2_bottom - (38 + 3) * scale, 130, 38, scale)) {
		;
	}

	// 989, 733
	if (frame_btn(col, "?", menubar_right - (30 + 5) * scale, menubar2_bottom - (30 + 5) * scale, 30, 30, scale)) {
		;
	}

	if (ImGui::BeginPopup("EditMenuPopup")) {
		if (ImGui::MenuItem("Quit")) {
			e->next_menu_state = MenuState::start;
			ImGui::CloseCurrentPopup();
		}

		//if (ImGui::MenuItem("Save")) {}

		if (ImGui::MenuItem("Load"))
			fd.Open();

		ImGui::EndPopup();
	}

	if (frame_btn(col, "Menu", menubar_right - (60 + 3) * scale, btn_y + 3 * scale, 60, 40, scale)) {
		e->sfx.play_sfx(SfxId::sfx_ui_click);
		ImGui::OpenPopup("EditMenuPopup");
	}

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
}

}

}
