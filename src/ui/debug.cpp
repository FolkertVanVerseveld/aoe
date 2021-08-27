#include "all.hpp"

#include "../engine.hpp"

#include <atomic>

extern std::atomic_bool running;

namespace genie {

extern AoE *eng;

namespace ui {

void TexturesWidget::display() {
	if (!visible)
		return;

	Texture &tex = eng->tex_bkg;
	ImVec2 sz(300, 200);
	ImGui::SetNextWindowSizeConstraints(sz, ImVec2(FLT_MAX, FLT_MAX));

	ImGui::Begin("Texture map", &visible);
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			ImTextureID id = (ImTextureID)tex.tex;
			ImGui::Image(id, ImVec2(tex.ts.bnds.w, tex.ts.bnds.h));
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

DebugUI::DebugUI() : tex(), unitview(), show_unitview(false) {}

void DebugUI::display() {
	if (!ImGui::BeginMainMenuBar())
		return;

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("About")) {}

		ImGui::Separator();

		if (ImGui::MenuItem("Quit"))
			running = false;

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View")) {
		ImGui::Checkbox("Graphics debugger", &tex.visible);
		ImGui::Checkbox("Unit viewer", &show_unitview);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Settings")) {
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	tex.display();
	if (unitview.get())
		unitview->show(show_unitview);
}

UI::UI() : m_show_debug(false), dbg() {}

void UI::show_debug(bool enabled) {
	m_show_debug = enabled;
}

void UI::display() {
	if (m_show_debug)
		dbg.display();
}

static const char *unit_states_lst[] = {
	"decomposing", "dying", "waiting", "moving", "attacking",
};

const std::map<std::string, UnitInfo> unit_info{
	{"missingno", {226, 342, 345, 444, 686, "pharaoh jason"}},
	{"archer1", {203, 308, 367, 413, 652, "bowman"}},
	{"archer2", {204, 309, 368, 414, 653, "improved bowman"}},
	{"archer3", {223, 337, 398, 439, 681, "composite bowman"}},
	{"cavarcher1", {202, 310, 369, 412, 650, "chariot archer"}},
	{"cavarcher2", {209, 318, 377, 422, 661, "horse archer"}},
	{"cavarcher3", {214, 323, 385, 427, 666, "elephant archer"}},
	{"infantry1", {212, 321, 380, 425, 664, "clubman"}},
	{"infantry2", {205, 311, 370, 415, 654, "axeman"}},
	{"infantry3", {206, 312, 371, 416, 655, "swordsman"}},
	{"infantry4", {220, 334, 395, 437, 678, "broad swordsman"}},
	{"infantry5", {219, 333, 394, 436, 677, "long swordsman"}},
	{"infantry6", {225, 340, 401, 442, 684, "hoplite"}},
	{"infantry7", {221, 335, 396, 438, 679, "phalanx"}},
	{"siege1", {629, 317, 376, 421, 660, "stone thrower"}},
	{"siege2", {208, 316, 375, 420, 659, "catapult"}},
	{"siege3", {207, 313, 372, 417, 656, "ballista"}},
	{"cavalry1", {227, 343, 403, 445, 651, "scout"}},
	{"cavalry2", {211, 320, 379, 424, 663, "cavalry"}}, // decomposing is bogus
	{"cavalry3", {210, 319, 378, 423, 662, "cataphract"}},
	{"cavalry4", {213, 322, 381, 426, 665, "chariot"}},
	{"elephant1", {216, 325, 387, 429, 669, "elephant"}},
	{"elephant2", {228, 446, 446, 446, 687, "tamed elephant"}},
	{"priest", {634, 341, 402, 443, 685, "priest"}},
	{"worker1", {224, 314, 373, 418, 657, "villager"}},
	{"worker2", {473, 473, 473, 473, 473, "fishing boat"}},
	{"worker3", {474, 474, 474, 474, 474, "fishing ship"}},
	{"worker4", {639, 639, 639, 639, 639, "merchant boat"}},
	{"worker5", {640, 640, 640, 640, 640, "merchant ship"}},
	{"artifact2", {244, "artifact"}}, // player owned
	{"cheat1", {604, 606, 605, 607, 603, "storm trooper"}},
	{"cheat2", {345, 345, 345, 715, 713, "storm trooper"}},
	// gaia
	{"elephant", {215, 324, 386, 428, 667, "elephant"}},
	{"gaia1", {217, 330, 391, 433, 673, "alligator"}}, // 477 swimming
	{"gaia2", {222, 336, 397, 497, 528, "lion"}}, // 680 moving1
	{"cow", {434, 345, 345, 434, 674, "cow"}},
	{"horse", {475, 345, 345, 475, 475, "horse"}},
	{"deer", {331, 331, 392, 479, 480, "deer"}}, // 478 moving1
	{"artifact", {245, "artifact"}},
	{"bird1", {406, 345, 345, 406, 404, "eagle"}},
	{"bird2", {521, 345, 345, 521, 518, "bird"}},
};

std::vector<const char*> unit_info_lst;

UnitViewWidget::UnitViewWidget()
	: type(0), anim()
	, pal(genie::assets.blobs[2]->open_pal((unsigned)genie::DrsId::palette), SDL_FreePalette)
	, preview(), player(0), offx(0), offy(0), subimage(0), state(0), changed(true), flip(false), info(false)
{
	if (unit_info_lst.empty()) {
		for (auto &kv : unit_info) {
			unit_info_lst.emplace_back(kv.first.c_str());
		}
	}
}

void UnitViewWidget::compute_offset() {
	offx = offy = 0;

	for (unsigned i = 0, n = anim->image_count; i < n; ++i) {
		const genie::Image &img = anim->subimage(i, player);
		offx = std::max(offx, img.bnds.x);
		offy = std::max(offy, img.bnds.y);
	}
}

void UnitViewWidget::set_anim(const UnitInfo &info, UnitState state) {
	genie::res_id id = -1;

	switch (state) {
		case UnitState::attack: id = info.attack_id; break;
		case UnitState::dead: id = info.decay_id; break;
		case UnitState::die: id = info.dead_id; break;
		case UnitState::idle: id = info.idle_id; break;
		case UnitState::moving: id = info.moving_id; break;
		default:
			throw std::runtime_error("bad unit state");
	}

	anim = std::move(genie::assets.blobs[1]->open_anim(id));
	compute_offset();
	this->info = &info;
	changed = true;
}

void UnitViewWidget::show(bool &visible) {
	if (!visible)
		return;

	if (ImGui::Begin("Unit viewer", &visible)) {
		int rel_offx, rel_offy;

		int old_type = type;
		changed |= ImGui::Combo("type", &type, unit_info_lst.data(), unit_info_lst.size());

		if (changed && type >= 0 && type < unit_info_lst.size()) {
			std::string key(unit_info_lst[type]);
			auto it = unit_info.find(key);
			set_anim(it->second, (UnitState)std::clamp(state, 0, (int)UnitState::max - 1));
		}

		if (!anim.has_value()) {
			ImGui::TextUnformatted("no type selected");
		} else {
			changed |= ImGui::Combo("action", &state, unit_states_lst, IM_ARRAYSIZE(unit_states_lst));
			if (changed && state >= 0 && state < IM_ARRAYSIZE(unit_states_lst)) {
				std::string key(unit_info_lst[type]);
				auto it = unit_info.find(key);
				set_anim(it->second, (UnitState)state);
			}

			ImGui::Text("name: %s", info->name.c_str());
			ImGui::Text("max offx: %d, max offy: %d", offx, offy);
			changed |= ImGui::Checkbox("flip", &flip);
			changed |= ImGui::SliderInputInt("subimage", &subimage, -anim->image_count + 1, 2 * anim->image_count);
			if (subimage < 0)
				subimage += anim->image_count;
			if (subimage >= anim->image_count)
				subimage %= anim->image_count;

			if (changed) {
				const genie::Image &img = anim->subimage(subimage, player);

				if (flip) {
					genie::Image imgf(std::move(img.flip()));

					preview.load(imgf, pal.get());

					rel_offx = offx - imgf.bnds.x;
					rel_offy = offy - imgf.bnds.y;
				} else {
					preview.load(img, pal.get());

					rel_offx = offx - img.bnds.x;
					rel_offy = offy - img.bnds.y;
				}

				preview.offx = rel_offx;
				preview.offy = rel_offy;
			}

			preview.show();
		}

		changed = false;
	}
	ImGui::End();
}

}
}
