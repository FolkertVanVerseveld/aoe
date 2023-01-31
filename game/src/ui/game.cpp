#include "../engine.hpp"

namespace aoe {

using namespace ui;

void UICache::show_terrain() {
	ZoneScoped;

	Assets &a = *e->assets.get();

	const ImageSet &s_desert = a.anim_at(io::DrsId::trn_desert);
	const ImageSet &s_grass = a.anim_at(io::DrsId::trn_grass);
	const ImageSet &s_water = a.anim_at(io::DrsId::trn_water);
	const ImageSet &s_deepwater = a.anim_at(io::DrsId::trn_deepwater);

	const gfx::ImageRef &t0 = a.at(s_desert.imgs[0]);
	const gfx::ImageRef &t1 = a.at(s_grass.imgs[0]);
	const gfx::ImageRef &t2 = a.at(s_water.imgs[0]);
	const gfx::ImageRef &t3 = a.at(s_deepwater.imgs[0]);

	const gfx::ImageRef tt[] = { t0, t1, t2, t3 };

	GameView &gv = e->gv;

	for (int y = 0; y < gv.t.h; ++y) {
		for (int x = 0; x < gv.t.w; ++x) {
			ImU32 col = IM_COL32_WHITE;

			uint8_t id = gv.t.id_at(x, y);
			uint8_t h = gv.t.h_at(x, y);
			if (!id) {
				// draw black tile
				col = IM_COL32_BLACK;
			}

			id %= 4;

			const gfx::ImageRef &img = tt[id];

			ImVec2 tpos(e->tilepos(x, y, left, top, h));

			float x0 = tpos.x - img.hotspot_x;
			float y0 = tpos.y - img.hotspot_y;

			bkg->AddImage(e->tex1, ImVec2(x0, y0), ImVec2(x0 + img.bnds.w, y0 + img.bnds.h), ImVec2(img.s0, img.t0), ImVec2(img.s1, img.t1), col);
		}
	}
}

void UICache::show_world() {
	ZoneScoped;

	ImGuiIO &io = ImGui::GetIO();

	show_terrain();

	if (!io.WantCaptureMouse)
		user_interact_entities();

	show_entities();
	show_selections();
}

#undef small

static bool menu_btn(ImTextureID tex, UICache &ui, const Assets &a, const char *lbl, float x, float scale, bool small) {
	ZoneScoped;
	const ImageSet &btns_s = a.anim_at(small ? io::DrsId::gif_menu_btn_small0 : io::DrsId::gif_menu_btn_medium0);
	
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();
	const gfx::ImageRef &rbtns = a.at(btns_s.imgs[0]);

	float btn_w = rbtns.bnds.w * scale, btn_h = rbtns.bnds.h * scale;
	float y = vp->WorkPos.y + 2 * scale;

	float s0 = rbtns.s0, t0 = rbtns.t0, s1 = rbtns.s1, t1 = rbtns.t1;
	bool held = false;

	if (io.MousePos.x >= x && io.MousePos.x < x + btn_w && io.MousePos.y >= vp->WorkPos.y && io.MousePos.y < vp->WorkPos.y + btn_h) {
		auto &r = a.at(btns_s.imgs[1]);
		s0 = r.s0; t0 = r.t0; s1 = r.s1; t1 = r.t1;
		held = true;
	}

	lst->AddImage(tex, ImVec2(x, vp->WorkPos.y), ImVec2(x + btn_w, vp->WorkPos.y + btn_h), ImVec2(s0, t0), ImVec2(s1, t1));

	ImVec2 sz(ImGui::CalcTextSize(lbl));
	if (held) { ++x; ++y; }
	ui.str2(ImVec2(x + (btn_w - sz.x) / 2, y), lbl);

	// TODO use held for sfx and image

	if (held && io.MouseDown[0] && io.MouseDownDuration[0] <= 0.0f)
		return true;

	return false;
}

void Engine::show_multiplayer_game() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *assets.get();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	// only use custom mouse if not hovering any imgui elements
	if (!io.WantCaptureMouse) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		sdl->set_cursor(1);
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	}

	float menubar_bottom = vp->WorkPos.y;

	const ImageSet &s = a.anim_at(io::DrsId::gif_menubar0);
	const gfx::ImageRef &rtop = a.at(s.imgs[0]), &rbottom = a.at(s.imgs[1]);

	// TODO /scale/ui.scale/
	float scale = std::max(1.0f, font_scaling ? io.DisplaySize.y / WINDOW_HEIGHT_MAX : 1.0f);

	float menubar_left = vp->WorkPos.x;

	// align center if menubar smaller than screen dimensions
	float menubar_w = rtop.bnds.w * scale;
	float menubar_h = rtop.bnds.h * scale;

	if (vp->WorkSize.x > menubar_w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - menubar_w) / 2;

	if (keyctl.is_tapped(GameKey::toggle_chat) && !show_chat)
		show_chat = true;

	// TODO fetch from player view
	int food = 200, wood = 200, gold = 0, stone = 150;
	std::string age(txt(StrId::age_stone));

	lst->AddImage(tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_left + menubar_w, vp->WorkPos.y + menubar_h), ImVec2(rtop.s0, rtop.t0), ImVec2(rtop.s1, rtop.t1));

	char buf[16];
	snprintf(buf, sizeof buf, "%d", food);
	buf[(sizeof buf) - 1] = '\0';

	float y = vp->WorkPos.y + 2 * scale;

	ui.str2(ImVec2(menubar_left + 32 * scale, y), buf);

	snprintf(buf, sizeof buf, "%d", wood);
	buf[(sizeof buf) - 1] = '\0';

	ui.str2(ImVec2(menubar_left + 99 * scale, y), buf);

	snprintf(buf, sizeof buf, "%d", gold);
	buf[(sizeof buf) - 1] = '\0';

	ui.str2(ImVec2(menubar_left + 166 * scale, y), buf);

	snprintf(buf, sizeof buf, "%d", stone);
	buf[(sizeof buf) - 1] = '\0';

	ui.str2(ImVec2(menubar_left + 234 * scale, y), buf);

	ImVec2 sz(ImGui::CalcTextSize(age.c_str()));

	ui.str2(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) / 2, y), age.c_str());

	// draw menu button
	
	const ImageSet &btns_s = a.anim_at(io::DrsId::gif_menu_btn_small0);
	const gfx::ImageRef &rbtns = a.at(btns_s.imgs[0]);

	float menubar_right = std::min(io.DisplaySize.x, menubar_left + menubar_w);

	float btn_w = rbtns.bnds.w * scale;
	float btn_left = menubar_right - btn_w;

	const ImageSet &btnm_s = a.anim_at(io::DrsId::gif_menu_btn_medium0);
	const gfx::ImageRef &rbtnm = a.at(btnm_s.imgs[0]);

	if (menu_btn(tex1, ui, a, "Menu", btn_left, scale, true)) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		ImGui::OpenPopup("MenuPopup");
	}

	if (ImGui::BeginPopup("MenuPopup")) {
		if (ImGui::MenuItem("Achievements"))
			show_achievements = !show_achievements;

		if (ImGui::MenuItem("Quit"))
			cancel_multiplayer_host(MenuState::defeat);

		ImGui::EndPopup();
	}

	btn_left -= rbtnm.bnds.w * scale;
	if (menu_btn(tex1, ui, a, "Diplomacy", btn_left, scale, false)) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		show_diplomacy = !show_diplomacy;
	}

	btn_left -= rbtns.bnds.w * scale;
	if (menu_btn(tex1, ui, a, "Chat", btn_left, scale, true)) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		show_chat = !show_chat;
	}

	menubar_bottom += menubar_h;

	if (show_chat)
	{
		Frame f;

		ImGui::SetNextWindowPos(ImVec2(menubar_left, menubar_bottom));

		if (f.begin("Chat", show_chat, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse)) {
			ImGui::SetWindowSize(ImVec2(400, 0));

			{
				Child ch;

				if (ch.begin("ChatHistory", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * 0.8f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))

					for (std::string &s : chat)
						ImGui::TextWrapped("%s", s.c_str());

				ImGui::SetScrollHereY(1.0f);
			}

			show_chat_line(f);
		}
	}

	menubar_w = rbottom.bnds.w * scale;
	menubar_h = rbottom.bnds.h * scale;

	float top = vp->WorkPos.y + vp->WorkSize.y - menubar_h;

	lst->AddImage(tex1, ImVec2(menubar_left, top), ImVec2(menubar_left + menubar_w, top + menubar_h), ImVec2(rbottom.s0, rbottom.t0), ImVec2(rbottom.s1, rbottom.t1));


	ui.show_hud_selection(menubar_left, top, menubar_h);

	if (show_achievements)
		show_multiplayer_achievements();

	if (show_diplomacy)
		show_multiplayer_diplomacy();

	if (cv.gameover) {
		FontGuard fg(fnt.fnt_copper2);

		const char *txt = "Game Over";

		ImVec2 sz(ImGui::CalcTextSize(txt));

		lst->AddText(ImVec2((io.DisplaySize.x - sz.x) / 2, (io.DisplaySize.y - sz.y) / 2), IM_COL32_WHITE, txt);
	}
}

}
