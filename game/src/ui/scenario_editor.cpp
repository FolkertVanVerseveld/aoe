#include "../ui.hpp"

#include "../engine.hpp"

#include <string.hpp>

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

// load local strings
struct ScnBtn {
	std::string lbl;
	int x, y;
} scn_btns[] = {
	{"Map", 0, 0},
	{"Terrain", 1, 0},
	{"Players", 2, 0},
	{"Units", 3, 0},
	{"Diplomacy", 4, 0},
	{"Individual Victory", 0, 1},
	{"Global Victory", 1, 1},
	{"Options", 2, 1},
	{"Messages", 3, 1},
	{"Cinematics", 4, 1},
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

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
	float menubar_w = bkg.bnds.w * scale;

	// align center if menubar smaller than screen dimensions
	if (vp->WorkSize.x > menubar_w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - menubar_w) / 2;

	float menubar_right = std::min(io.DisplaySize.x, menubar_left + menubar_w);

	float t1, h = 50.0f;

	t1 = bkg.t0 + (bkg.t1 - bkg.t0) * h / bkg.bnds.h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_right, vp->WorkPos.y + h * scale), ImVec2(bkg.s0, bkg.t0), ImVec2(bkg.s1, t1));

	h = 143.0f;
	float t0 = bkg.t0 + (bkg.t1 - bkg.t0) * (bkg.bnds.h - h) / bkg.bnds.h;

	float menubar2_top = vp->WorkPos.y + vp->WorkSize.y - h * scale;
	float menubar2_bottom = vp->WorkPos.y + vp->WorkSize.y;

	lst->AddImage(e->tex1, ImVec2(menubar_left, menubar2_top), ImVec2(menubar_right, menubar2_bottom), ImVec2(bkg.s0, t0), ImVec2(bkg.s1, bkg.t1));

	// draw buttons
	BackgroundColors col;

	col = a.bkg_cols.at(io::DrsId::bkg_editor_menu);

	const float padding = 2;
	// XXX times scale or not?
	float btn_x = menubar_left + padding * scale;
	float btn_y = menubar_top + padding * scale;

	float btn_w = 110, btn_h = 22;

	// show menubar top buttons
	for (unsigned i = 0; i < ARRAY_SIZE(scn_btns); ++i) {
		const ScnBtn &btn = scn_btns[i];

		if (frame_btn(col, btn.lbl.c_str(), btn_x + (btn_w + padding) * btn.x * scale, btn_y + (btn_h + padding) * btn.y * scale, btn_w, btn_h, scale, scn_edit.top_btn_idx == i)) {
			e->sfx.play_sfx(SfxId::sfx_ui_click);
			scn_edit.top_btn_idx = i;
		}
	}

	if (ImGui::BeginPopup("EditMenuPopup")) {
		if (ImGui::MenuItem("Quit")) {
			e->next_menu_state = MenuState::start;
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem("Save"))
			fd2.Open();

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
			// TODO use scn_edit to load everything
			try {
				scn.load(path.c_str());
				set_scn(scn);
			} catch (io::BadScenarioError &e) {
				scn_edit.load(path.c_str());
			}
			e->cam_reset();
		} catch (std::runtime_error &e) {
			fprintf(stderr, "%s: cannot load scn: %s\n", __func__, e.what());
		}
		fd.ClearSelected();
	}

	fd2.Display();

	if (fd2.HasSelected()) {
		std::string path(fd2.GetSelected().string());
		try {
			scn_edit.save(path);
		} catch (std::runtime_error &e) {
			fprintf(stderr, "%s: cannot save scn: %s\n", __func__, e.what());
		}
		fd2.ClearSelected();
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

	if (!f.begin("ScnEditBottom", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	ImGui::SetWindowPos(ImVec2(menubar_left, menubar2_top));
	ImGui::SetWindowSize(ImVec2(menubar_right - menubar_left, menubar2_bottom - menubar2_top));

	// bottom menubar ui stuff
	switch (scn_edit.top_btn_idx) {
	case 0:
		// 11, 636 - 625
		//str2(ImVec2(menubar_left + 11 * scale, menubar2_top + 11 * scale), "Map");

		ImGui::InputInt("map type", &scn_edit.map_gen_type);

		// 182, 725 - 625

		//if (frame_btn(col, "Generate Map", menubar_left + 182 * scale, menubar2_bottom - (38 + 3) * scale, 130, 38, scale)) {}

		ImGui::InputInt("map width", &scn_edit.map_gen_width);
		ImGui::InputInt("map height", &scn_edit.map_gen_height);

		ImGui::InputInt("default terrain", &scn_edit.map_gen_terrain_type);

		if (f.btn("Generate Map")) {
			e->sfx.play_sfx(SfxId::sfx_ui_click);
			scn_edit.create_map();
		}
		break;
	default:
		str2(ImVec2(menubar_left + 11 * scale, menubar2_top + 11 * scale), "Work in progress");
		break;
	}

	// help button
	// 989, 733
	if (frame_btn(col, "?", menubar_right - (30 + 5) * scale, menubar2_bottom - (30 + 5) * scale, 30, 30, scale)) {
		;
	}
}

}

}
