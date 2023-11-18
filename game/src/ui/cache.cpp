#include "../ui.hpp"

#include "../debug.hpp"
#include "../engine.hpp"

namespace aoe {

namespace ui {

UICache::UICache()
	: civs(), e(nullptr), entities(), entities_deceased(), particles(), selected(), display_area()
	, left(0), top(0), scale(1)
	, bkg(nullptr), btnsel()
	, t_imgs()
	, fd(), fd2(ImGuiFileBrowserFlags_EnterNewFilename)
	, scn(), scn_edit(), scn_game(), mem(), gmb_top(), gmb_bottom()
	, select_started(false), multi_select(false), btn_left(false), build_menu(-1), start_x(0), start_y(0) {}

static constexpr bool point_in_rect(float x, float y, const SDL_Rect &rect)
{
	return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

unsigned UICache::player_index() const {
	return e->cv.playerindex;
}

void UICache::mouse_left_process() {
	ZoneScoped;

	ImGuiIO &io = ImGui::GetIO();

	bool old = btn_left;
	btn_left = io.MouseDown[0];

	float dx = 0, dy = 0;

	if (select_started) {
		dx = start_x - io.MousePos.x;
		dy = start_y - io.MousePos.y;
	}

	if (fabs(dx) > 3 || fabs(dy) > 3)
		multi_select = true;

	// stop if not changed
	if (old == btn_left || io.MouseDownDuration[0] > 0.0f)
		return;

	if (io.MouseDown[0]) {
		select_started = true;
		multi_select = false;
		start_x = io.MousePos.x;
		start_y = io.MousePos.y;
		return;
	}

	bool select_area = multi_select;
	multi_select = select_started = false;

	ImGuiViewport *vp = ImGui::GetMainViewport();

	if (!select_area && (point_in_rect(io.MousePos.x, io.MousePos.y, gmb_top) || point_in_rect(io.MousePos.x, io.MousePos.y, gmb_bottom)))
		return;

	build_menu = -1;

	if (select_area) {
		SDL_Rect area;

		area.x = (int)std::min(start_x, io.MousePos.x);
		area.y = (int)std::min(start_y, io.MousePos.y);
		area.w = (int)std::max(start_x, io.MousePos.x) - area.x;
		area.h = (int)std::max(start_y, io.MousePos.y) - area.y;

		collect(this->selected, area);
	} else {
		collect(this->selected, io.MousePos.x, io.MousePos.y);
	}

	if (this->selected.empty())
		return;

	if (select_area) {
		// filter all non controllable entities
		bool has_units = false, has_buildings = false;

		for (auto it = this->selected.begin(); it != this->selected.end();) {
			IdPoolRef ref = *it;
			Entity *ent = e->gv.try_get(ref);

			if (!ent || player_index() != ent->playerid) {
				it = this->selected.erase(it);
				continue;
			}

			if (is_building(ent->type))
				has_buildings = true;
			else if (!is_resource(ent->type))
				has_units = true;

			++it;
		}

		// prefer selecting units over buildings
		if (has_units) {
			for (auto it = this->selected.begin(); it != this->selected.end();) {
				IdPoolRef ref = *it;
				Entity *ent = e->gv.try_get(ref);

				if (is_building(ent->type))
					it = this->selected.erase(it);
				else
					++it;
			}
		} else if (has_buildings) {
			for (auto it = this->selected.begin(); it != this->selected.end();) {
				IdPoolRef ref = *it;
				Entity *ent = e->gv.try_get(ref);

				if (!is_building(ent->type))
					it = this->selected.erase(it);
				else
					++it;
			}
		}

		if (this->selected.empty())
			return;
	} else {
		this->selected.resize(1);
	}

	IdPoolRef ref = *this->selected.begin();
	Entity *ent = e->gv.try_get(ref);
	assert(ent);

	SfxId sfx = SfxId::sfx_ui_click;

	if (ent) {
		switch (ent->type) {
		case EntityType::town_center:
			sfx = SfxId::towncenter;
			break;
		case EntityType::barracks:
			sfx = SfxId::barracks;
			break;
		case EntityType::villager:
		case EntityType::melee1:
		case EntityType::priest:
			sfx = SfxId::villager_random;
			break;
		}
	}

	// only play if valid and we can control the entity
	if (sfx != SfxId::sfx_ui_click && player_index() == ent->playerid)
		e->sfx.play_sfx(sfx);
}

void UICache::mouse_right_process() {
	ZoneScoped;

	ImGuiIO &io = ImGui::GetIO();

	if (!(io.MouseDown[1] && io.MouseDownDuration[1] <= 0.0f))
		return;

	if (selected.empty())
		return;

	// find visual tile
	Assets &a = *e->assets.get();
	const gfx::ImageRef &t0 = a.at(t_imgs[0].imgs[0]);
	const gfx::ImageRef &t1 = a.at(t_imgs[1].imgs[0]);
	const gfx::ImageRef &t2 = a.at(t_imgs[2].imgs[0]);
	const gfx::ImageRef &t3 = a.at(t_imgs[3].imgs[0]);

	const gfx::ImageRef tt[] = { t0, t1, t2, t3 };
	GameView &gv = e->gv;

	ImGuiViewport *vp = ImGui::GetMainViewport();

	float mx = io.MousePos.x, my = io.MousePos.y;

	//printf("right click at %.2f,%.2f\n", mx, my);

	if (!selected.empty()) {
		std::vector<IdPoolRef> targets;
		collect(targets, mx, my);

		if (!targets.empty()) {
			IdPoolRef t = targets.front();

			for (IdPoolRef r : selected) {
				Entity &ent = e->gv.get(r);
				e->client->entity_infer(ent.ref, t);
			}

			return;
		}
	}

	// no entities found to interact with, search tiles
	for (VisualTile &vt : display_area) {
		if (mx >= vt.bnds.x && mx < vt.bnds.x + vt.bnds.w && my >= vt.bnds.y && my < vt.bnds.y + vt.bnds.h) {
			// get actual image
			uint8_t id = gv.t.tile_at(vt.tx, vt.ty);
			uint8_t h = gv.t.h_at(vt.tx, vt.ty);
			if (!id) {
				// TODO does not work yet :/
				// unexplored tiles should still be clickable
				// use dummy id, which may be sligthly inaccurate
				id = 0;
				continue;
			}

			const gfx::ImageRef &r = imgtile(id);

			int y = (int)(my - vt.bnds.y);

			// ignore if out of range
			if (y < 0 || y >= r.mask.size())
				continue;

			auto row = r.mask.at(y);
			int x = (int)(mx - vt.bnds.x);

			// ignore if point not on line
			if (x < row.first || x >= row.second)
				continue;

			// TODO determine precise value, now we always use tile's center
			//printf("clicked on %d,%d\n", vt.tx, vt.ty);

			for (IdPoolRef ref : selected) {
				Entity *ent = e->gv.try_get(ref);
				if (!ent)
					continue;

				e->client->entity_move(ent->ref, vt.tx, vt.ty);
			}
			return;
		}
	}

	//printf("nothing selected\n");
}

}

}
