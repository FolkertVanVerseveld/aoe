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
		if (!ent.is_alive() && is_building(ent.type)) {
			const EntityBldInfo &info = ent.bld_info();

			int x = ent.x, y = ent.y;
			uint8_t h = e->gv.t.h_at(x, y);

			ImVec2 tpos(e->tilepos(ent.x + 1, ent.y, left, top, h));
			float x0, y0;

			const gfx::ImageRef &tcp = a.at(info.slp_die);

			x0 = tpos.x - tcp.hotspot_x;
			y0 = tpos.y - tcp.hotspot_y;

			entities_deceased.emplace_back(ent.ref, tcp.ref, x0, y0, tcp.bnds.w, tcp.bnds.h, tcp.s0, tcp.t0, tcp.s1, tcp.t1, tpos.y + 0.1f, ent.xflip);
		} else if (ent.state != EntityState::decaying) {
			continue;
		} else if (is_resource(ent.type)) {
			// resources cannot decay
			assert("decaying resource" == NULL);
		} else {
			// figure out orientation and animation
			float x = ent.x, y = ent.y;
			int ix = (int)x, iy = (int)y;
			uint8_t h = e->gv.t.h_at(ix, iy);
			ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

			assert(ent.state == EntityState::decaying);

			const EntityImgInfo &info = ent.img_info();
			io::DrsId gif = info.slp_decay;

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
		if (ent.state == EntityState::decaying)
			continue;

		if (is_building(ent.type)) {
			const EntityBldInfo &info = ent.bld_info();

			int x = ent.x, y = ent.y;
			uint8_t h = e->gv.t.h_at(x, y);

			ImVec2 tpos(e->tilepos(ent.x + 1, ent.y, left, top, h));
			float x0, y0;

			if (info.slp_player != info.slp_base) {
				const ImageSet &s_tc = a.anim_at(info.slp_base);
				IdPoolRef imgref = s_tc.imgs[0];
				const gfx::ImageRef &tc = a.at(imgref);

				x0 = tpos.x - tc.hotspot_x;
				y0 = tpos.y - tc.hotspot_y;

				entities.emplace_back(ent.ref, imgref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
			}

			const ImageSet &s_tcp = a.anim_at(info.slp_player);
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

				entities.emplace_back(ent.ref, fimg.ref, x0, y0, fimg.bnds.w, fimg.bnds.h, fimg.s0, fimg.t0, fimg.s1, fimg.t1, tpos.y + 0.2f, false);
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
				case EntityType::grass_tree1:
					img = io::DrsId::ent_grass_tree1;
					break;
				case EntityType::grass_tree2:
					img = io::DrsId::ent_grass_tree2;
					break;
				case EntityType::grass_tree3:
					img = io::DrsId::ent_grass_tree3;
					break;
				case EntityType::grass_tree4:
					img = io::DrsId::ent_grass_tree4;
					break;
				case EntityType::dead_tree1:
					img = ent.state == EntityState::decaying ? io::DrsId::ent_decay_tree : io::DrsId::ent_dead_tree1;
					break;
				case EntityType::dead_tree2:
					img = ent.state == EntityState::decaying ? io::DrsId::ent_decay_tree : io::DrsId::ent_dead_tree2;
					break;
				case EntityType::gold:
				case EntityType::stone: {
					io::DrsId gif = ent.type == EntityType::gold ? io::DrsId::gif_gold : io::DrsId::gif_stone;
					const ImageSet &s_tc = a.anim_at(gif);
					IdPoolRef imgref = s_tc.try_at(ent.subimage);
					const gfx::ImageRef &tc = a.at(imgref);

					float x0 = tpos.x - tc.hotspot_x;
					float y0 = tpos.y - tc.hotspot_y;

					if (ent.xflip) {
						x0 = tpos.x - tc.bnds.w + tc.hotspot_x;
						entities.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s1, tc.t0, tc.s0, tc.t1, tpos.y, ent.xflip);
					} else {
						entities.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
					}

					continue;
				}
			}

			const gfx::ImageRef &tc = a.at(img);

			float x0 = tpos.x - tc.hotspot_x;
			float y0 = tpos.y - tc.hotspot_y;

			entities.emplace_back(ent.ref, tc.ref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y, ent.xflip);
		} else {
			// figure out orientation and animation
			float x = ent.x, y = ent.y;
			int ix = (int)x, iy = (int)y;
			uint8_t h = e->gv.t.h_at(ix, iy);
			ImVec2 tpos(e->tilepos(x + 1, y, left, top, h));

			io::DrsId gif = io::DrsId::gif_bird1;
			const EntityImgInfo &info = ent.img_info();

			switch (ent.state) {
			case EntityState::dying:         gif = info.slp_die;           break;
			case EntityState::decaying:      gif = info.slp_decay;         break;
			case EntityState::attack:        gif = info.slp_attack;        break;
			case EntityState::attack_follow: gif = info.slp_attack_follow; break;
			case EntityState::moving:        gif = info.slp_moving;        break;
			default:                         gif = info.slp_alive;         break;
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
