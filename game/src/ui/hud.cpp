#include "../ui.hpp"

#include "../engine.hpp"
#include "../world/entity_info.hpp"

#include "../nominmax.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::idle() {
	ZoneScoped;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	scale = std::max(1.0f, e->font_scaling ? io.DisplaySize.y / WINDOW_HEIGHT_MAX : 1.0f);
	left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(e->cam_x);
	top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(e->cam_y);

	bkg = ImGui::GetBackgroundDrawList();

	auto &killed = e->gv.entities_killed;

	if (killed.empty())
		return;

	for (const EntityView &ev : killed) {
		// TODO filter if out of camera range
		if (is_building(ev.type)) {
			e->sfx.play_sfx(SfxId::bld_die_random);
			continue;
		}

		switch (ev.type) {
		default:
			e->sfx.play_sfx(SfxId::villager_die_random);
			break;
		}
	}

	killed.clear();

	for (unsigned pos : e->gv.players_died) {
		PlayerView &pv = e->gv.players[pos];
		e->add_chat_text(pv.init.name + " resigned");
		e->sfx.play_sfx(SfxId::player_resign);
	}
}

void UICache::user_interact_entities() {
	ZoneScoped;

	if (e->keyctl.is_tapped(GameKey::kill_entity) && !selected.empty()) {
		e->client->entity_kill(*selected.begin());
		return;
	}
}

void UICache::show_hud_selection(float menubar_left, float top, float menubar_h) {
	ZoneScoped;
	if (selected.empty())
		return;

	IdPoolRef ref = *selected.begin();
	Entity *ent = e->gv.try_get(ref);
	if (!ent)
		return;

	// 5, 649 -> 5, 7
	bkg->AddRectFilled(ImVec2(menubar_left + 8 * scale, top + 7 * scale), ImVec2(menubar_left + 131 * scale, top + menubar_h - 7 * scale), IM_COL32_BLACK);

	// TODO determine description
	// 8, 650 -> 5, 8
	const EntityInfo &info = entity_info.at((unsigned)ent->type);

	unsigned icon = info.icon;
	const char *name = info.name.c_str();

	bkg->AddText(ImVec2(menubar_left + 8 * scale, top + 8 * scale), IM_COL32_WHITE, "Egyptian");
	// 664 -> 22
	bkg->AddText(ImVec2(menubar_left + 8 * scale, top + 22 * scale), IM_COL32_WHITE, name);

	Assets &a = *e->assets.get();
	const ImageSet &s_bld = a.anim_at(is_building(ent->type) ? io::DrsId::gif_building_icons : io::DrsId::gif_unit_icons);
	// TODO fetch player color
	const gfx::ImageRef &img = a.at(s_bld.try_at(ent->color, icon));

	// 8,679 -> 8,37
	float x0 = menubar_left + 10 * scale, y0 = top + 37 * scale;
	bkg->AddImage(e->tex1, ImVec2(x0, y0), ImVec2(x0 + img.bnds.w * scale, y0 + img.bnds.h * scale), ImVec2(img.s0, img.t0), ImVec2(img.s1, img.t1));

	// HP
	// 8, 744 -> 8,102
	char buf[32];

	snprintf(buf, sizeof buf, "%u/%u", 0, info.hp);
	bkg->AddText(ImVec2(menubar_left + 8 * scale, top + 102 * scale), IM_COL32_WHITE, buf);
}

}

}
