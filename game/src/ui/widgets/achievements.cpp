#include "../../engine.hpp"
#include "../../ui.hpp"

namespace aoe {

using namespace ui;

bool Engine::show_achievements(Frame &f, bool bkg) {
	ZoneScoped;
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *assets.get();
	const gfx::ImageRef &ref = a.at(io::DrsId::bkg_achievements);

	ImDrawList *lst = ImGui::GetWindowDrawList();
	ImVec2 pos(ImGui::GetWindowPos());
	ImVec2 tl(ImGui::GetWindowContentRegionMin()), br(ImGui::GetWindowContentRegionMax());
	tl.x += pos.x; tl.y += pos.y; br.x += pos.x; br.y += pos.y;

	float w = 1, sx = 1, sy = 1;

	if (bkg) {
		lst->AddImage(tex1, tl, br, ImVec2(ref.s0, ref.t0), ImVec2(ref.s1, ref.t1));
		w = br.x - tl.x;
	} else {
		//w = ImGui::GetMainViewport()->Size.x;
		w = io.DisplaySize.x;
		if (w < 1) {
			int iw, dummy;
			sdl->window.size(iw, dummy);
			w = iw;
		}
	}

	sx = w / ref.bnds.w;
	sy = (br.y - tl.y) / ref.bnds.h;

	if (show_timeline) {
		f.str("World Population");

		// TODO make timeline
		f.str("Work in progress...");

		f.str("Time");

		if (f.btn("Back")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			show_timeline = false;
		}
	} else {
		// TODO populate table

		f.str("Work in progress...");
		{
			// military at 160, 244
			// economy at 250, 212
			// religion at 346, 180
			// technology at 578, 148 (center)
			// survival at 810, 180
			// wonder at 910, 212
			// total score at 996, 244
			lst->AddText(ImVec2(tl.x + 160 * sx, tl.y + 244 * sy), IM_COL32_WHITE, "Military");
			lst->AddText(ImVec2(tl.x + 250 * sx, tl.y + 212 * sy), IM_COL32_WHITE, "Economy");
			lst->AddText(ImVec2(tl.x + 346 * sx, tl.y + 180 * sy), IM_COL32_WHITE, "Religion");

			ImVec2 sz;

			sz = ImGui::CalcTextSize("Technology");
			lst->AddText(ImVec2(tl.x + 578 * sx - sz.x / 2, tl.y + 148 * sy), IM_COL32_WHITE, "Technology");

			sz = ImGui::CalcTextSize("Survival");
			lst->AddText(ImVec2(tl.x + 810 * sx - sz.x, tl.y + 180 * sy), IM_COL32_WHITE, "Survival");

			sz = ImGui::CalcTextSize("Wonder");
			lst->AddText(ImVec2(tl.x + 908 * sx - sz.x, tl.y + 212 * sy), IM_COL32_WHITE, "Wonder");

			sz = ImGui::CalcTextSize("Total Score");
			lst->AddText(ImVec2(tl.x + 996 * sx - sz.x, tl.y + 244 * sy), IM_COL32_WHITE, "Total Score");

			// 40, 290
			// 40, 338
			// 40, 386

#if 0
			for (unsigned i = 0; i < cv.scn.players.size(); ++i) {
				PlayerSetting &p = cv.scn.players[i];

				float rowy = tl.y + 300 * sy + (348 - 300) * i * sy;

				lst->AddText(ImVec2(tl.x + 40 * sx, rowy - sz.y), IM_COL32_WHITE, p.name.c_str());

				sz = ImGui::CalcTextSize("0");
				lst->AddText(ImVec2(tl.x + 324 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, "0");

				sz = ImGui::CalcTextSize("0");
				lst->AddText(ImVec2(tl.x + 408 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, "0");

				sz = ImGui::CalcTextSize("0");
				lst->AddText(ImVec2(tl.x + 492 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, "0");

				sz = ImGui::CalcTextSize("0");
				lst->AddText(ImVec2(tl.x + 577 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, "0");

				sz = ImGui::CalcTextSize("Yes");
				lst->AddText(ImVec2(tl.x + 662 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, "Yes");

				sz = ImGui::CalcTextSize("100");
				lst->AddText(ImVec2(tl.x + 830 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, "100");
			}
#else
			for (unsigned i = 1; i < gv.players.size(); ++i) {
				PlayerView &v = gv.players[i];

				float rowy = tl.y + 300 * sy + (348 - 300) * (i - 1) * sy;

				std::string txt;

				txt = v.init.name;

				lst->AddText(ImVec2(tl.x + 40 * sx, rowy - sz.y), IM_COL32_WHITE, txt.c_str());

				txt = v.alive ? "Yes" : "No";

				sz = ImGui::CalcTextSize(txt.c_str());
				lst->AddText(ImVec2(tl.x + 662 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, txt.c_str());

				txt = std::to_string(v.military);

				sz = ImGui::CalcTextSize(txt.c_str());
				lst->AddText(ImVec2(tl.x + 324 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, txt.c_str());

				txt = std::to_string(v.score);

				sz = ImGui::CalcTextSize(txt.c_str());
				lst->AddText(ImVec2(tl.x + 830 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, txt.c_str());
			}
#endif
		}

		if (f.btn("Timeline")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			show_timeline = true;
		}

		f.sl();

		if (f.btn("Close")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			return false;
		}
	}

	return true;
}

void Engine::show_multiplayer_achievements() {
	ImGuiIO &io = ImGui::GetIO();

	float scale = io.DisplaySize.y / WINDOW_HEIGHT_MIN;

	ImVec2 max(std::min(io.DisplaySize.x, WINDOW_WIDTH_MAX + 8.0f), std::min(io.DisplaySize.y, WINDOW_HEIGHT_MAX + 8.0f));

	ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(max.x, 450 * scale), std::min(max.y, 420 * scale)), max);
	Frame f;

	if (!f.begin("Achievements", m_show_achievements))
		return;

	if (!show_achievements(f, true))
		m_show_achievements = false;
}

}
