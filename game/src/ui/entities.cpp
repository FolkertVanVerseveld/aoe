#include "../ui.hpp"

#include "../engine.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::load_entities() {
	ZoneScoped;
	
	ImGuiViewport *vp = ImGui::GetMainViewport();
	Assets &a = *e->assets.get();

	// TODO collect dead entities first
	// TODO refactor this so we don't have to copy paste both for loops

	for (const Entity &ent : e->gv.entities) {
		if (ent.is_alive())
			continue;

		// TODO use particle rather than dead building entity
		if (is_building(ent.type)) {
			io::DrsId bld_base = io::DrsId::bld_debris;

			int x = ent.x, y = ent.y;
			uint8_t h = e->gv.t.h_at(x, y);

			ImVec2 tpos(e->tilepos(ent.x + 1, ent.y, left, top, h));
			float x0, y0;

			const gfx::ImageRef &tcp = a.at(bld_base);

			x0 = tpos.x - tcp.hotspot_x;
			y0 = tpos.y - tcp.hotspot_y;

			entities_deceased.emplace_back(ent.ref, tcp.ref, x0, y0, tcp.bnds.w, tcp.bnds.h, tcp.s0, tcp.t0, tcp.s1, tcp.t1, tpos.y + 0.1f, ent.xflip);
		} else if (is_resource(ent.type)) {
			float x = ent.x, y = ent.y;
			int ix = (int)x, iy = (int)y;
			uint8_t h = e->gv.t.h_at(ix, iy);
			ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

			io::DrsId img = io::DrsId::ent_desert_tree1;

			switch (ent.type) {
			case EntityType::berries:
				img = io::DrsId::ent_berries;
				break;
			case EntityType::desert_tree1:
				img = io::DrsId::ent_desert_tree1;
				break;
			case EntityType::desert_tree2:
				img = io::DrsId::ent_desert_tree2;
				break;
			case EntityType::desert_tree3:
				img = io::DrsId::ent_desert_tree3;
				break;
			case EntityType::desert_tree4:
				img = io::DrsId::ent_desert_tree4;
				break;
			case EntityType::dead_tree1:
				img = ent.state == EntityState::decaying ? io::DrsId::ent_decay_tree : io::DrsId::ent_dead_tree1;
				break;
			case EntityType::dead_tree2:
				img = ent.state == EntityState::decaying ? io::DrsId::ent_decay_tree : io::DrsId::ent_dead_tree2;
				break;
			}

			const gfx::ImageRef &tc = a.at(img);

			float x0 = tpos.x - tc.hotspot_x;
			float y0 = tpos.y - tc.hotspot_y;

			entities_deceased.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
		} else {
			// TODO figure out orientation and animation
			float x = ent.x, y = ent.y;
			int ix = (int)x, iy = (int)y;
			uint8_t h = e->gv.t.h_at(ix, iy);
			ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

			io::DrsId gif = io::DrsId::gif_bird1;

			// TODO use lookup table and refactor into function
			switch (ent.type) {
			case EntityType::bird1:
				break;
			case EntityType::villager:
				switch (ent.state) {
				case EntityState::dying:
					gif = io::DrsId::gif_villager_die1;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_villager_decay;
					break;
				case EntityState::attack:
					gif = io::DrsId::gif_villager_attack;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_villager_move;
					break;
				default:
					gif = io::DrsId::gif_villager_stand;
					break;
				}
				break;
			case EntityType::worker_wood1:
			case EntityType::worker_wood2:
				switch (ent.state) {
				case EntityState::dying:
					gif = io::DrsId::gif_worker_wood_die;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_worker_wood_decay;
					break;
				case EntityState::attack:
					if (ent.type == EntityType::worker_wood1)
						gif = io::DrsId::gif_worker_wood_attack1;
					else
						gif = io::DrsId::gif_worker_wood_attack2;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_worker_wood_move;
					break;
				default:
					gif = io::DrsId::gif_worker_wood_stand;
					break;
				}
				break;
			case EntityType::melee1:
				switch (ent.state) {
				case EntityState::dying:
					gif = io::DrsId::gif_melee1_die;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_melee1_decay;
					break;
				case EntityState::attack:
					gif = io::DrsId::gif_melee1_attack;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_melee1_move;
					break;
				default:
					gif = io::DrsId::gif_melee1_stand;
					break;
				}
				break;
			case EntityType::priest:
				switch (ent.state) {
				case EntityState::attack:
					gif = io::DrsId::gif_priest_attack;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_priest_move;
					break;
				case EntityState::dying:
					gif = io::DrsId::gif_priest_die;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_priest_decay;
					break;
				default:
					gif = io::DrsId::gif_priest_stand;
					break;
				}
				break;
			}

			const ImageSet &s_tc = a.anim_at(gif);
			IdPoolRef imgref = s_tc.try_at(ent.playerid, ent.subimage);
			const gfx::ImageRef &tc = a.at(imgref);

			float x0 = tpos.x - tc.hotspot_x;
			float y0 = tpos.y - tc.hotspot_y;

			if (ent.xflip) {
				x0 = tpos.x - tc.bnds.w + tc.hotspot_x;
				entities_deceased.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s1, tc.t0, tc.s0, tc.t1, tpos.y, ent.xflip);
			} else {
				entities_deceased.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
			}
		}
	}

	for (const Entity &ent : e->gv.entities) {
		if (!ent.is_alive())
			continue;

		if (is_building(ent.type)) {
			io::DrsId bld_base = io::DrsId::bld_town_center;
			io::DrsId bld_player = io::DrsId::bld_town_center_player;

			switch (ent.type) {
			case EntityType::barracks:
				bld_base = bld_player = io::DrsId::bld_barracks;
				break;
			}

			int x = ent.x, y = ent.y;
			uint8_t h = e->gv.t.h_at(x, y);

			ImVec2 tpos(e->tilepos(ent.x + 1, ent.y, left, top, h));
			float x0, y0;

			if (bld_player != bld_base) {
				const ImageSet &s_tc = a.anim_at(bld_base);
				IdPoolRef imgref = s_tc.imgs[0];
				const gfx::ImageRef &tc = a.at(imgref);

				x0 = tpos.x - tc.hotspot_x;
				y0 = tpos.y - tc.hotspot_y;

				entities.emplace_back(ent.ref, imgref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
			}

			const ImageSet &s_tcp = a.anim_at(bld_player);
			IdPoolRef imgref = s_tcp.at(ent.playerid, 0);
			const gfx::ImageRef &tcp = a.at(imgref);

			x0 = tpos.x - tcp.hotspot_x;
			y0 = tpos.y - tcp.hotspot_y;

			entities.emplace_back(ent.ref, imgref, x0, y0, tcp.bnds.w, tcp.bnds.h, tcp.s0, tcp.t0, tcp.s1, tcp.t1, tpos.y + 0.1f, ent.xflip);

			// if building is damaged, add fire
			float hper = (float)ent.stats.hp / ent.stats.maxhp;

			if (hper <= 0.75f) {
				io::DrsId fid = io::DrsId::gif_bld_fire3;

				if (hper <= 0.25f)
					fid = io::DrsId::gif_bld_fire1;
				else if (hper <= 0.5f)
					fid = io::DrsId::gif_bld_fire2;

				const ImageSet &s_fire = a.anim_at(fid);
				const gfx::ImageRef &fimg = a.at(s_fire.try_at(ent.subimage));

				x0 = tpos.x - fimg.hotspot_x;
				y0 = tpos.y - fimg.hotspot_y;

				entities.emplace_back(ent.ref, fimg.ref, x0, y0, fimg.bnds.w, fimg.bnds.h, fimg.s0, fimg.t0, fimg.s1, fimg.t1, tpos.y + 0.1f, false);
			}
		} else if (is_resource(ent.type)) {
			float x = ent.x, y = ent.y;
			int ix = (int)x, iy = (int)y;
			uint8_t h = e->gv.t.h_at(ix, iy);
			ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

			io::DrsId img = io::DrsId::ent_desert_tree1;

			switch (ent.type) {
			case EntityType::berries:
				img = io::DrsId::ent_berries;
				break;
			case EntityType::desert_tree1:
				img = io::DrsId::ent_desert_tree1;
				break;
			case EntityType::desert_tree2:
				img = io::DrsId::ent_desert_tree2;
				break;
			case EntityType::desert_tree3:
				img = io::DrsId::ent_desert_tree3;
				break;
			case EntityType::desert_tree4:
				img = io::DrsId::ent_desert_tree4;
				break;
			case EntityType::dead_tree1:
				img = ent.state == EntityState::decaying ? io::DrsId::ent_decay_tree : io::DrsId::ent_dead_tree1;
				break;
			case EntityType::dead_tree2:
				img = ent.state == EntityState::decaying ? io::DrsId::ent_decay_tree : io::DrsId::ent_dead_tree2;
				break;
			}

			const gfx::ImageRef &tc = a.at(img);

			float x0 = tpos.x - tc.hotspot_x;
			float y0 = tpos.y - tc.hotspot_y;

			entities.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
		} else {
			// TODO figure out orientation and animation
			float x = ent.x, y = ent.y;
			int ix = (int)x, iy = (int)y;
			uint8_t h = e->gv.t.h_at(ix, iy);
			ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

			io::DrsId gif = io::DrsId::gif_bird1;

			// TODO use lookup table and refactor into function
			switch (ent.type) {
			case EntityType::bird1:
				break;
			case EntityType::villager:
				switch (ent.state) {
				case EntityState::dying:
					gif = io::DrsId::gif_villager_die1;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_villager_decay;
					break;
				case EntityState::attack:
					gif = io::DrsId::gif_villager_attack;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_villager_move;
					break;
				default:
					gif = io::DrsId::gif_villager_stand;
					break;
				}
				break;
			case EntityType::worker_wood1:
			case EntityType::worker_wood2:
				switch (ent.state) {
				case EntityState::dying:
					gif = io::DrsId::gif_worker_wood_die;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_worker_wood_decay;
					break;
				case EntityState::attack:
					if (ent.type == EntityType::worker_wood1)
						gif = io::DrsId::gif_worker_wood_attack1;
					else
						gif = io::DrsId::gif_worker_wood_attack2;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_worker_wood_move;
					break;
				default:
					gif = io::DrsId::gif_worker_wood_stand;
					break;
				}
				break;
			case EntityType::melee1:
				switch (ent.state) {
				case EntityState::dying:
					gif = io::DrsId::gif_melee1_die;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_melee1_decay;
					break;
				case EntityState::attack:
					gif = io::DrsId::gif_melee1_attack;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_melee1_move;
					break;
				default:
					gif = io::DrsId::gif_melee1_stand;
					break;
				}
				break;
			case EntityType::priest:
				switch (ent.state) {
				case EntityState::attack:
					gif = io::DrsId::gif_priest_attack;
					break;
				case EntityState::attack_follow:
				case EntityState::moving:
					gif = io::DrsId::gif_priest_move;
					break;
				case EntityState::dying:
					gif = io::DrsId::gif_priest_die;
					break;
				case EntityState::decaying:
					gif = io::DrsId::gif_priest_decay;
					break;
				default:
					gif = io::DrsId::gif_priest_stand;
					break;
				}
				break;
			}

			const ImageSet &s_tc = a.anim_at(gif);
			IdPoolRef imgref = s_tc.try_at(ent.playerid, ent.subimage);
			const gfx::ImageRef &tc = a.at(imgref);

			float x0 = tpos.x - tc.hotspot_x;
			float y0 = tpos.y - tc.hotspot_y;

			if (ent.xflip) {
				x0 = tpos.x - tc.bnds.w + tc.hotspot_x;
				entities.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s1, tc.t0, tc.s0, tc.t1, tpos.y, ent.xflip);
			} else {
				entities.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
			}
		}
	}
}

}

}
