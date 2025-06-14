#include "../engine.hpp"

namespace aoe {

using namespace ui;

void UICache::draw_tile(uint8_t id, uint8_t h, int x, int y, const ImVec2 &size, ImU32 col) {
	const gfx::ImageRef &img = imgtile(id);

	ImVec2 tpos(e->tilepos(x, y, left, top, h));

	float x0 = tpos.x - img.hotspot_x;
	float y0 = tpos.y - img.hotspot_y;

	float x1 = x0 + img.bnds.w;
	float y1 = y0 + img.bnds.h;

	if (x1 >= 0 && x0 < size.x && y1 >= 0 && y0 < size.y) {
		if (x == 0 && y == 0)
			--tpos.x;

		display_area.emplace_back(x, y, SDL_Rect{ (int)x0, (int)y0, (int)img.bnds.w + 1, (int)img.bnds.h + 1 });

		bkg->AddImage(e->tex1, ImVec2(x0, y0), ImVec2(x1, y1), ImVec2(img.s0, img.t0), ImVec2(img.s1, img.t1), col);
	}
}

void UICache::show_terrain() {
	ZoneScoped;

	Assets &a = *e->assets.get();
	ImGuiIO &io = ImGui::GetIO();

	GameView &gv = e->gv;

	display_area.clear();

	for (int y = 0; y < gv.t.h; ++y) {
		for (int x = 0; x < gv.t.w; ++x) {
			ImU32 col = IM_COL32_WHITE;

			uint16_t id = gv.t.tile_at(x, y);
			uint8_t h = gv.t.h_at(x, y);
			if (!id) {
				// draw black tile
				col = IM_COL32_BLACK;
			}

			if (Terrain::tile_hasoverlay(id)) {
				TileType base = Terrain::tile_base(id);
				unsigned meta = Terrain::tile_img(id);
				unsigned bits = meta >> (12 - 3);
				unsigned img = meta & ~0x1e00;

				unsigned base_id = Terrain::tile_id(base, img);
				draw_tile(base_id, h, x, y, io.DisplaySize, col);

				TileType t = Terrain::tile_type(id);

				assert(bits);

				if (bits & 1)
					draw_tile(Terrain::tile_id(t, 0), h, x, y, io.DisplaySize, col);
				if (bits & 2)
					draw_tile(Terrain::tile_id(t, 1), h, x, y, io.DisplaySize, col);
				if (bits & 4)
					draw_tile(Terrain::tile_id(t, 2), h, x, y, io.DisplaySize, col);
				if (bits & 8)
					draw_tile(Terrain::tile_id(t, 3), h, x, y, io.DisplaySize, col);
			} else {
				draw_tile(id, h, x, y, io.DisplaySize, col);
			}
		}
	}
}

void UICache::show_world() {
	ZoneScoped;
	show_terrain();
	show_entities();
	show_selections();
	show_particles();
}

void UICache::show_particles() {
	ZoneScoped;
	particles.clear();

	ImGuiViewport *vp = ImGui::GetMainViewport();
	Assets &a = *e->assets.get();

	for (const Particle &p : e->gv.particles) {
		float x = p.x, y = p.y;
		int ix = (int)x, iy = (int)y;
		uint8_t h = e->gv.t.h_at(ix, iy);
		ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

		io::DrsId gif = io::DrsId::gif_moveto;

		switch (p.type) {
		case ParticleType::explode1: gif = io::DrsId::gif_explode1; break;
		case ParticleType::explode2: gif = io::DrsId::gif_explode2; break;
		case ParticleType::moveto:
			break;
		default:
			assert("bad type");
			break;
		}

		const ImageSet &s_gif = a.anim_at(gif);
		IdPoolRef imgref = s_gif.try_at(p.subimage);
		const gfx::ImageRef &tc = a.at(imgref);

		float x0 = tpos.x - tc.hotspot_x;
		float y0 = tpos.y - tc.hotspot_y;

		particles.emplace_back(p.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, false);
	}

	std::sort(particles.begin(), particles.end(), [](const VisualEntity &lhs, const VisualEntity &rhs) { return lhs.z < rhs.z; });

	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	for (VisualEntity &v : particles) {
		lst->AddImage(e->tex1, ImVec2((int)v.x, (int)v.y), ImVec2(int(v.x + v.w), int(v.y + v.h)), ImVec2(v.s0, v.t0), ImVec2(v.s1, v.t1));
	}
}

bool UICache::menu_btn(ImTextureID tex, const Assets &a, const char *lbl, float x, float scale, bool small) {
	ZoneScoped;
	const ImageSet &btns_s = a.anim_at(small ? io::DrsId::gif_menu_btn_small0 : io::DrsId::gif_menu_btn_medium0);

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();
	const gfx::ImageRef &rbtns = a.at(btns_s.imgs[0]);

	float btn_w = rbtns.bnds.w * scale, btn_h = rbtns.bnds.h * scale;
	float y = vp->WorkPos.y + 2 * scale;

	float s0 = rbtns.s0, t0 = rbtns.t0, s1 = rbtns.s1, t1 = rbtns.t1;
	bool held = false, snd = false;

	if (io.MousePos.x >= x && io.MousePos.x < x + btn_w && io.MousePos.y >= vp->WorkPos.y && io.MousePos.y < vp->WorkPos.y + btn_h) {
		if (btnsel == lbl && io.MouseDown[0]) {
			auto &r = a.at(btns_s.imgs[1]);
			s0 = r.s0; t0 = r.t0; s1 = r.s1; t1 = r.t1;
			snd = true;
		}
		held = true;
	}

	lst->AddImage(tex, ImVec2(x, vp->WorkPos.y), ImVec2(x + btn_w, vp->WorkPos.y + btn_h), ImVec2(s0, t0), ImVec2(s1, t1));

	ImVec2 sz(ImGui::CalcTextSize(lbl));
	if (snd) { ++x; ++y; }
	str2(ImVec2(x + (btn_w - sz.x) / 2, y), lbl);

	if (held && io.MouseDown[0] && io.MouseDownDuration[0] <= 0.0f)
		btnsel = lbl;

	if (!io.MouseDown[0] && btnsel == lbl) {
		btnsel.clear();
		if (held)
			return true;
	}

	return false;
}

bool UICache::frame_btn(const BackgroundColors &col, const char *lbl, float x, float y, float w, float h, float scale, bool invert) {
	ImDrawList *lst = ImGui::GetBackgroundDrawList();
	ImGuiIO &io = ImGui::GetIO();

	float btn_w = w * scale, btn_h = h * scale;
	GLfloat right = x + btn_w, bottom = y + btn_h;

	// TODO select logic
	bool held = false, snd = false;

	if (io.MousePos.x >= x && io.MousePos.x < right && io.MousePos.y >= y && io.MousePos.y < bottom) {
		if (btnsel == lbl && io.MouseDown[0])
			snd = true;

		held = true;
	}

	SDL_Color coltbl[6];

	if (snd) {
		coltbl[0] = col.border[2];
		coltbl[1] = col.border[1];
		coltbl[2] = col.border[0];
		coltbl[3] = col.border[5];
		coltbl[4] = col.border[4];
		coltbl[5] = col.border[3];
	} else {
		coltbl[0] = col.border[0];
		coltbl[1] = col.border[1];
		coltbl[2] = col.border[2];
		coltbl[3] = col.border[3];
		coltbl[4] = col.border[4];
		coltbl[5] = col.border[5];
	}

	DrawLine(lst, x, y, right, y, coltbl[0]);
	DrawLine(lst, right - 1, y, right - 1, bottom - 1, coltbl[0]);

	DrawLine(lst, x + 1, y + 1, right - 1, y + 1, coltbl[1]);
	DrawLine(lst, right - 2, y + 1, right - 2, bottom - 2, coltbl[1]);

	DrawLine(lst, x + 2, y + 2, right - 2, y + 2, coltbl[2]);
	DrawLine(lst, right - 3, y + 2, right - 3, bottom - 3, coltbl[2]);

	DrawLine(lst, x, y, x, bottom, coltbl[5]);
	DrawLine(lst, x, bottom - 1, right, bottom - 1, coltbl[5]);

	DrawLine(lst, x + 1, y + 1, x + 1, bottom - 1, coltbl[4]);
	DrawLine(lst, x + 1, bottom - 2, right - 1, bottom - 2, coltbl[4]);

	DrawLine(lst, x + 2, y + 2, x + 2, bottom - 2, coltbl[3]);
	DrawLine(lst, x + 2, bottom - 3, right - 2, bottom - 3, coltbl[3]);

	ImVec2 sz(ImGui::CalcTextSize(lbl));
	if (snd) { ++x; ++y; }
	str2(ImVec2(x + (btn_w - sz.x) / 2, y + (btn_h - sz.y) / 2), lbl, invert);

	if (held && io.MouseDown[0] && io.MouseDownDuration[0] <= 0.0f)
		btnsel = lbl;

	if (!io.MouseDown[0] && btnsel == lbl) {
		btnsel.clear();
		if (held)
			return true;
	}

	return false;
}

void UICache::show_multiplayer_game() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *e->assets.get();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	// only use custom mouse if not hovering any imgui elements
	if (!io.WantCaptureMouse) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		e->sdl->set_cursor(1);
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	}

	float menubar_bottom = vp->WorkPos.y;

	const ImageSet &s = a.anim_at(io::DrsId::gif_menubar0);
	const gfx::ImageRef &rtop = a.at(s.imgs[0]), &rbottom = a.at(s.imgs[1]);

	// TODO /scale/ui.scale/
	float scale = std::max(1.0f, e->font_scaling ? io.DisplaySize.y / WINDOW_HEIGHT_MAX : 1.0f);

	float menubar_left = vp->WorkPos.x;

	// align center if menubar smaller than screen dimensions
	float menubar_w = rtop.bnds.w * scale;
	float menubar_h = rtop.bnds.h * scale;

	if (vp->WorkSize.x > menubar_w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - menubar_w) / 2;

	// TODO fetch from player view
	PlayerView &p = e->gv.players.at(e->cv.playerindex);

	// TODO use p.res.age
	std::string age(e->txt(StrId::age_stone));

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_left + menubar_w, vp->WorkPos.y + menubar_h), ImVec2(rtop.s0, rtop.t0), ImVec2(rtop.s1, rtop.t1));

	gmb_top.x = menubar_left;
	gmb_top.w = menubar_w;
	gmb_top.y = vp->WorkPos.y;
	gmb_top.h = menubar_h;

	float y = vp->WorkPos.y + 2 * scale;

	strnum(ImVec2(menubar_left + 32 * scale, y), p.res.wood);
	strnum(ImVec2(menubar_left + 99 * scale, y), p.res.food);
	strnum(ImVec2(menubar_left + 166 * scale, y), p.res.gold);
	strnum(ImVec2(menubar_left + 234 * scale, y), p.res.stone);

	ImVec2 sz(ImGui::CalcTextSize(age.c_str()));

	str2(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) / 2, y), age.c_str());

	// draw menu button
	
	const ImageSet &btns_s = a.anim_at(io::DrsId::gif_menu_btn_small0);
	const gfx::ImageRef &rbtns = a.at(btns_s.imgs[0]);

	float menubar_right = std::min(io.DisplaySize.x, menubar_left + menubar_w);

	float btn_w = rbtns.bnds.w * scale;
	float btn_left = menubar_right - btn_w;

	const ImageSet &btnm_s = a.anim_at(io::DrsId::gif_menu_btn_medium0);
	const gfx::ImageRef &rbtnm = a.at(btnm_s.imgs[0]);

	if (ImGui::BeginPopup("MenuPopup")) {
		if (ImGui::MenuItem("Achievements"))
			e->m_show_achievements = !e->m_show_achievements;

		if (ImGui::MenuItem("Quit")) {
			e->quit_game(e->cv.victory ? MenuState::victory : MenuState::defeat);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// TODO convert into retained gui elements
	if (menu_btn(e->tex1, a, "Menu", btn_left, scale, true)) {
		e->sfx.play_sfx(SfxId::ui_click);
		ImGui::OpenPopup("MenuPopup");
	}

	btn_left -= rbtnm.bnds.w * scale;
	if (menu_btn(e->tex1, a, "Diplomacy", btn_left, scale, false)) {
		e->sfx.play_sfx(SfxId::ui_click);
		e->show_diplomacy = !e->show_diplomacy;
	}

	btn_left -= rbtns.bnds.w * scale;
	if (menu_btn(e->tex1, a, "Chat", btn_left, scale, true)) {
		e->sfx.play_sfx(SfxId::ui_click);
		e->show_chat = !e->show_chat;
	}

	menubar_bottom += menubar_h;

	if (e->show_chat)
	{
		Frame f;

		ImGui::SetNextWindowPos(ImVec2(menubar_left, menubar_bottom));

		if (f.begin("Chat", e->show_chat, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse)) {
			ImGui::SetWindowSize(ImVec2(400, 0));

			{
				Child ch;

				if (ch.begin("ChatHistory", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * 0.8f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
					for (std::string &s : e->chat)
						ImGui::TextWrapped("%s", s.c_str());

				ImGui::SetScrollHereY(1.0f);
			}

			e->show_chat_line(f);
		}
	}

	menubar_w = rbottom.bnds.w * scale;
	menubar_h = rbottom.bnds.h * scale;

	float top = vp->WorkPos.y + vp->WorkSize.y - menubar_h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, top), ImVec2(menubar_left + menubar_w, top + menubar_h), ImVec2(rbottom.s0, rbottom.t0), ImVec2(rbottom.s1, rbottom.t1));

	gmb_bottom.x = menubar_left;
	gmb_bottom.w = menubar_w;
	gmb_bottom.y = top;
	gmb_bottom.h = menubar_h;

	show_hud_selection(menubar_left, top, menubar_h);

	if (e->m_show_achievements)
		e->show_multiplayer_achievements();

	if (e->show_diplomacy)
		e->show_multiplayer_diplomacy();

	if (e->cv.gameover) {
		str_scream(e->cv.victory ? "Victory" : "Game Over");
	} else if (!e->client->is_running()) {
		str_scream("Game Paused");
	}
}

}
