#include "../ui.hpp"

#include "../engine.hpp"

#include "../nominmax.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::idle() {
	ZoneScoped;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	scale = std::max(1.0f, e->font_scaling ? io.DisplaySize.y / WINDOW_HEIGHT_MAX : 1.0f);
	left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(e->cam_x) - 0.5f;
	top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(e->cam_y) - 0.5f;

	bkg = ImGui::GetBackgroundDrawList();

	auto &killed = e->gv.entities_killed;

	if (killed.empty())
		return;

	for (const EntityView &ev : killed) {
		// TODO filter if out of camera range
		//if (is_building(ev.type))
		e->sfx.play_sfx(SfxId::bld_die_random);
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
	const char *name = "???";

	io::BldIcon icon = io::BldIcon::house1;

	// TODO refactor to Entity::name() or something
	switch (ent->type) {
	case EntityType::town_center:
		name = "Town Center";
		icon = io::BldIcon::towncenter1;
		break;
	case EntityType::barracks:
		name = "Barracks";
		icon = io::BldIcon::barracks1;
		break;
	case EntityType::villager:
		name = "Villager";
		break;
	case EntityType::bird1:
		name = "Bird";
		break;
	}

	bkg->AddText(ImVec2(menubar_left + 8 * scale, top + 8 * scale), IM_COL32_WHITE, "Egyptian");
	// 664 -> 22
	bkg->AddText(ImVec2(menubar_left + 8 * scale, top + 22 * scale), IM_COL32_WHITE, name);

	Assets &a = *e->assets.get();
	const ImageSet &s_bld = a.anim_at(io::DrsId::gif_building_icons);
	// TODO fetch player color
	const gfx::ImageRef &img = a.at(s_bld.imgs.at((unsigned)icon));

	// 8,679 -> 8,37
	float x0 = menubar_left + 10 * scale, y0 = top + 37 * scale;
	bkg->AddImage(e->tex1, ImVec2(x0, y0), ImVec2(x0 + img.bnds.w, y0 + img.bnds.h), ImVec2(img.s0, img.t0), ImVec2(img.s1, img.t1));
}

}

}
