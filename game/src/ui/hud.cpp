#include "../ui.hpp"

#include "../engine.hpp"
#include "../world/entity_info.hpp"
#include "../legacy/legacy.hpp"

#include <minmax.hpp>

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::idle(Engine &e) {
	ZoneScoped;

	this->e = &e;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	scale = std::max(1.0f, e.font_scaling ? io.DisplaySize.y / WINDOW_HEIGHT_MAX : 1.0f);
	left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(e.cam_x);
	top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(e.cam_y);

	bkg = ImGui::GetBackgroundDrawList();
	gui = &io;
	aoe::ui::vp = vp;
}

void UICache::idle_editor(Engine &e) {
	e.gv.try_read(scn_game);
}

void UICache::play_sfx(SfxId id, int loops) {
	e->sfx.play_sfx(id, loops);
}

void UICache::idle_game() {
	ZoneScoped;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	scale = std::max(1.0f, e->font_scaling ? io.DisplaySize.y / WINDOW_HEIGHT_MAX : 1.0f);

	float old_left = left, old_top = top;

	left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(e->cam_x);
	top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(e->cam_y);

	// adjust selection area if the camera is moving
	float dx = left - old_left, dy = top - old_top;

	if (fabs(dx) > 0.5f || fabs(dy) > 0.5f) {
		start_x += dx;
		start_y += dy;
	}

	bkg = ImGui::GetBackgroundDrawList();

	auto &spawned = e->gv.entities_spawned;

	for (IdPoolRef ref : spawned) {
		auto it = e->gv.entities.find(ref);
		if (it == e->gv.entities.end())
			continue;

		const Entity &ent = *it;
		auto sfx = spawn_sfx.find(ent.type);
		if (sfx == spawn_sfx.end())
			LOGF(stderr, "%s: unknown spawn sfx for entity %u\n", __func__, (unsigned)ent.type);
		else
			play_sfx(sfx->second);
	}

	spawned.clear();

	auto &killed = e->gv.entities_killed;

	for (const EntityView &ev : killed) {
		// TODO filter if out of camera range
		if (is_building(ev.type)) {
			play_sfx(SfxId::bld_die_random);
			continue;
		}

		switch (ev.type) {
		default:
			play_sfx(SfxId::villager_die_random);
			break;
		}
	}

	killed.clear();

	for (unsigned pos : e->gv.players_died) {
		PlayerView &pv = e->gv.players[pos];
		e->add_chat_text(pv.init.name + " resigned");
		play_sfx(SfxId::player_resign);
	}
}

void UICache::try_kill_first_entity() {
	ZoneScoped;
	if (!selected.empty())
		e->client->entity_kill(*selected.begin());
}

void UICache::image(const gfx::ImageRef &ref, float x, float y, float scale) {
	bkg->AddImage(e->tex1, ImVec2(x, y), ImVec2(x + ref.bnds.w * scale, y + ref.bnds.h * scale), ImVec2(ref.s0, ref.t0), ImVec2(ref.s1, ref.t1));
}

std::optional<Entity> UICache::first_selected_entity() {
	ZoneScoped;
	if (selected.empty())
		return std::nullopt;

	IdPoolRef ref = *selected.begin();
	Entity *ent = e->gv.try_get(ref);
	return ent ? *ent : std::optional<Entity>(std::nullopt);
}

std::optional<Entity> UICache::first_selected_building() {
	ZoneScoped;
	if (selected.empty())
		return std::nullopt;

	for (const IdPoolRef &ref : selected) {
		Entity *ent = e->gv.try_get(ref);
		if (ent && is_building(ent->type))
			return *ent;
	}

	return std::nullopt;
}

std::optional<Entity> UICache::first_selected_worker() {
	ZoneScoped;
	if (selected.empty())
		return std::nullopt;

	for (const IdPoolRef &ref : selected) {
		Entity *ent = e->gv.try_get(ref);
		if (ent && is_worker(ent->type))
			return *ent;
	}

	return std::nullopt;
}

bool UICache::try_select(EntityType type, unsigned playerid) {
	for (VisualEntity &v : entities) {
		Entity *ent = e->gv.try_get(v.ref);
		if (!ent || ent->playerid != playerid || !ent->is_alive())
			continue;

		if (ent->is_type(type)) {
			selected.clear();
			selected.emplace_back(ent->ref);
			// TODO focus
			return true;
		}
	}

	return false;
}

bool UICache::find_idle_villager(unsigned playerid) {
	bool changed = false;

	for (VisualEntity &v : entities) {
		Entity *ent = e->gv.try_get(v.ref);
		if (!ent || ent->playerid != playerid || !is_worker(ent->type))
			continue;

		if (ent->state == EntityState::alive) {
			if (!changed) {
				changed = true;
				selected.clear();
			}
			selected.emplace_back(ent->ref);
		}
	}

	return changed;
}

void UICache::try_open_build_menu() {
	if (state != HudState::worker)
		return;

	std::optional<Entity> ent = first_selected_worker();
	if (!ent.has_value())
		return;

	play_sfx(SfxId::hud_click);
	build_menu = 0;
}

void UICache::show_hud_selection(float menubar_left, float top, float menubar_h) {
	ZoneScoped;
	state = HudState::start;

	std::optional<Entity> ent = first_selected_entity();
	if (!ent.has_value())
		return;

	// 5, 649 -> 5, 7
	bkg->AddRectFilled(ImVec2(menubar_left + 8 * scale, top + 7 * scale), ImVec2(menubar_left + 131 * scale, top + menubar_h - 7 * scale), IM_COL32_BLACK);

	// TODO determine description
	// 8, 650 -> 5, 8
	const EntityInfo &info = entity_info.at((unsigned)ent->type);

	unsigned icon = info.icon;
	const char *name = info.name.c_str();

	bkg->AddText(ImVec2(menubar_left + 10 * scale, top + 8 * scale), IM_COL32_WHITE, "Egyptian"); // TODO add civ
	// 664 -> 22
	bkg->AddText(ImVec2(menubar_left + 10 * scale, top + 22 * scale), IM_COL32_WHITE, name);

	Assets &a = *e->assets.get();
	const ImageSet &s_bld = a.anim_at(is_building(ent->type) ? io::DrsId::gif_building_icons : io::DrsId::gif_unit_icons);
	const gfx::ImageRef &img = a.at(s_bld.try_at(ent->playerid, icon));

	// 8,679 -> 8,37
	float x0 = menubar_left + 10 * scale, y0 = top + 37 * scale;
	bkg->AddImage(e->tex1, ImVec2(x0, y0), ImVec2(x0 + img.bnds.w * scale, y0 + img.bnds.h * scale), ImVec2(img.s0, img.t0), ImVec2(img.s1, img.t1));

	// HP
	// 8, 733 -> 8,91
	unsigned hp = 0;
	unsigned subimage = std::clamp(25u * (ent->stats.maxhp - ent->stats.hp) / ent->stats.maxhp, 0u, 25u);

	const ImageSet &s_hpbar = a.anim_at(io::DrsId::gif_hpbar);
	const gfx::ImageRef &hpimg = a.at(s_hpbar.try_at(subimage));

	x0 = menubar_left + 10 * scale, y0 = top + 91 * scale;
	bkg->AddImage(e->tex1, ImVec2(x0, y0), ImVec2(x0 + hpimg.bnds.w * scale, y0 + hpimg.bnds.h * scale), ImVec2(hpimg.s0, hpimg.t0), ImVec2(hpimg.s1, hpimg.t1));

	// 8, 744 -> 8,102
	char buf[32];

	snprintf(buf, sizeof buf, "%u/%u", ent->stats.hp, ent->stats.maxhp);
	bkg->AddText(ImVec2(menubar_left + 10 * scale, top + 102 * scale), IM_COL32_WHITE, buf);

	if (e->cv.playerindex != ent->playerid)
		return;
	
	if (is_building(info.type)) {
		const ImageSet &s_units = a.anim_at(io::DrsId::gif_unit_icons);

		x0 = menubar_left + 140 * scale;
		y0 = top + 10 * scale;

		BackgroundColors col = a.bkg_cols.at(io::DrsId::bkg_editor_menu);

		switch (info.type) {
			case EntityType::town_center: {
				const EntityInfo &i_vil = entity_info.at((unsigned)EntityType::villager);
				const gfx::ImageRef &img_vil = a.at(s_units.try_at(ent->playerid, i_vil.icon));

				if (frame_btn(col, "train 1", x0 - 2, y0 - 2, img_vil.bnds.w * scale + 4, img_vil.bnds.h * scale + 4, 1)) {
					play_sfx(SfxId::hud_click);
					e->client->entity_train(ent->ref, EntityType::villager);
				}

				image(img_vil, x0, y0, scale);
				break;
			}
			case EntityType::barracks: {
				// TODO determine image based on age
				const EntityInfo &i_melee = entity_info.at((unsigned)EntityType::melee1);
				const gfx::ImageRef &img_melee = a.at(s_units.try_at(ent->playerid, i_melee.icon));

				if (frame_btn(col, "train 1", x0 - 2, y0 - 2, img_melee.bnds.w * scale + 4, img_melee.bnds.h * scale + 4, 1)) {
					play_sfx(SfxId::hud_click);
					e->client->entity_train(ent->ref, EntityType::melee1);
				}

				image(img_melee, x0, y0, scale);
				break;
			}
		}
	} else if (is_worker(ent->type)) {
		state = HudState::worker;

		const ImageSet &s_task = a.anim_at(io::DrsId::gif_task_icons);
		const gfx::ImageRef &img_build = a.at(s_task.imgs[2]);

		BackgroundColors col = a.bkg_cols.at(io::DrsId::bkg_editor_menu);

		x0 = menubar_left + 140 * scale;
		y0 = top + 10 * scale;

		if (build_menu < 0) {
			if (frame_btn(col, "build 1", x0 - 2, y0 - 2, img_build.bnds.w * scale + 4, img_build.bnds.h * scale + 4, 1)) {
				play_sfx(SfxId::hud_click);
				build_menu = 0; // TODO open build menu
			}

			image(img_build, x0, y0, scale);
		} else {
			state = HudState::buildmenu;
			const ImageSet &s_bld = a.anim_at(io::DrsId::gif_building_icons);
			unsigned p = ent->playerid;

			const gfx::ImageRef &bld_melee = a.at(s_bld.at(p, (unsigned)io::BldIcon::barracks1));
			const gfx::ImageRef &bld_docks = a.at(s_bld.at(p, (unsigned)io::BldIcon::dock1));
			const gfx::ImageRef &bld_food = a.at(s_bld.at(p, (unsigned)io::BldIcon::granary1));
			const gfx::ImageRef &bld_house = a.at(s_bld.at(p, (unsigned)io::BldIcon::house1));
			const gfx::ImageRef &bld_stock = a.at(s_bld.at(p, (unsigned)io::BldIcon::storage1));

			std::array<gfx::ImageRef, 5> blds{ bld_house, bld_melee, bld_food, bld_stock, bld_docks };

			float margin = 4;
			float w = blds[0].bnds.w * scale, h = blds[0].bnds.h * scale;
			float ww = w + margin + 4;

			for (unsigned i = 0; i < blds.size(); ++i) {
				char lbl[32];
				snprintf(lbl, sizeof lbl, "build %u", i);

				if (frame_btn(col, lbl, x0 - 2 + i * ww, y0 - 2, w + margin, h + margin, 1)) {
					play_sfx(SfxId::invalid_select);
					// TODO start building placement
				}

				image(blds[i], x0 + i * ww, y0, scale);
			}
		}
	}
}

}

}
