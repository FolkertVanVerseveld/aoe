#include "engine.hpp"

#include "sdl.hpp"

#include <cassert>
#include <cstdarg>
#include <cmath>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include "nominmax.hpp"
#include "ui.hpp"
#include "string.hpp"

#include <algorithm>

namespace aoe {

namespace ui {

void str(const char *s, TextHalign ha, bool wrap) {
	ImGui::TextUnformatted(s, (int)ha, wrap);
}

Frame::Frame() : open(false), active(false) {}

Frame::~Frame() {
	if (active) // frames always have to be closed
		close();
}

bool Frame::begin(const char *name, ImGuiWindowFlags flags) {
	assert(!open);
	open = ImGui::Begin(name, NULL, flags);
	active = true;
	return open;
}

bool Frame::begin(const char *name, bool &p_open, ImGuiWindowFlags flags) {
	assert(!open);
	open = ImGui::Begin(name, &p_open, flags);
	active = true;
	return open;
}

void Frame::close() {
	assert(active);
	ImGui::End();
	active = open = false;
}

void Frame::str2(const std::string &s, TextHalign ha, const ImVec4 &bg, bool wrap) {
	str2(s.c_str(), ha, bg, wrap);
}

void Frame::str2(const char *s, TextHalign ha, const ImVec4 &bg, bool wrap) {
	ImGui::PushStyleColor(ImGuiCol_Text, bg);
	ImGui::PushStyleColor(ImGuiCol_TextDisabled, bg);

	ImVec2 pos = ImGui::GetCursorPos();

	ImGui::SetCursorPos(ImVec2(pos.x - 1, pos.y + 1));
	ui::str(s, ha, wrap);

	ImGui::PopStyleColor(2);

	ImGui::SetCursorPos(pos);
	ui::str(s, ha, wrap);
}

void Frame::txt2(StrId id, TextHalign ha, const ImVec4 &bg) {
	std::string s(eng->txt(id));
	str2(s, ha, bg);
}

void Frame::str(const std::string &s) {
	str(s.c_str());
}

void Frame::str(const char *s) {
	ImGui::TextUnformatted(s);
}

void Frame::str(StrId id) {
	std::string s(eng->txt(id));
	str(s);
}

void Frame::fmt(const char *s, ...) {
	va_list args;
	va_start(args, s);

	ImGui::TextV(s, args);

	va_end(args);
}

bool chkbox(const char *s, bool &b) {
	return ImGui::Checkbox(s, &b);
}

bool Frame::chkbox(const char *s, bool &b) {
	return ui::chkbox(s, b);
}

bool Frame::btn(const char *s, TextHalign ha, const ImVec2 &sz) {
	return ImGui::Button(s, (int)ha, sz);
}

bool Frame::xbtn(const char *s, TextHalign ha, const ImVec2 &sz) {
	ImGui::BeginDisabled();
	bool b = ImGui::Button(s, (int)ha, sz);
	ImGui::EndDisabled();
	return b;
}

bool combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height) {
	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		bool b = ImGui::Combo(label, idx, lst, popup_max_height);
		ImGui::PopItemWidth();
		return b;
	}

	return ImGui::Combo(label, idx, lst, popup_max_height);
}

bool Frame::combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height) {
	return ui::combo(label, idx, lst, popup_max_height);
}

static bool scalar(const char *label, int32_t &v, int32_t step) {
	static_assert(sizeof(v) == sizeof(int32_t));
	int32_t v_old = v;
	bool b = false;

	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		b = ImGui::InputScalar(label, ImGuiDataType_S32, &v, step ? (const void *)&step : NULL);
		ImGui::PopItemWidth();
	} else {
		b = ImGui::InputScalar(label, ImGuiDataType_S32, &v, step ? (const void *)&step : NULL);
	}

	return b && v != v_old;
}

static bool scalar(const char *label, uint32_t &v, uint32_t step) {
	static_assert(sizeof(v) == sizeof(uint32_t));
	uint32_t v_old = v;
	bool b = false;

	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		b = ImGui::InputScalar(label, ImGuiDataType_U32, &v, step ? (const void *)&step : NULL);
		ImGui::PopItemWidth();
	} else {
		b = ImGui::InputScalar(label, ImGuiDataType_U32, &v, step ? (const void *)&step : NULL);
	}

	return b && v != v_old;
}

bool Frame::scalar(const char *label, int32_t &v, int32_t step) {
	return ui::scalar(label, v, step);
}

bool Frame::scalar(const char *label, uint32_t &v, uint32_t step) {
	return ui::scalar(label, v, step);
}

bool Frame::scalar(const char *label, uint32_t &v, uint32_t step, uint32_t min, uint32_t max) {
	v = std::clamp(v, min, max);
	uint32_t v_old = v;
	bool b = ui::scalar(label, v, step);
	v = std::clamp(v, min, max);
	return b && v != v_old;
}

static bool text(const char *label, std::string &buf, ImGuiInputTextFlags flags) {
	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		bool b = ImGui::InputText(label, buf, flags);
		ImGui::PopItemWidth();
		return b;
	}
	return ImGui::InputText(label, buf, flags);
}

bool Frame::text(const char *label, std::string &buf, ImGuiInputTextFlags flags) {
	return ui::text(label, buf, flags);
}

void Frame::sl() {
	ImGui::SameLine();
}

Child::Child() : open(false), active(false) {}

Child::~Child() {
	if (active) // children always have to be closed
		close();
}

bool Child::begin(const char *str_id, const ImVec2 &size, bool border, ImGuiWindowFlags extra_flags) {
	assert(!open);
	open = ImGui::BeginChild(str_id, size, border, extra_flags);
	active = true;
	return open;
}

void Child::close() {
	assert(active);
	ImGui::EndChild();
	active = open = false;
}

Table::Table() : open(false) {}

Table::~Table() {
	if (open)
		close();
}

bool Table::begin(const char *str_id, int columns, ImGuiTableFlags flags) {
	assert(!open);
	open = ImGui::BeginTable(str_id, columns, flags);
	return open;
}

void Table::close() {
	assert(open);
	ImGui::EndTable();
	open = false;
}

void Table::row(int id, std::initializer_list<const char*> lst) {
	ImGui::PushID(id);
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);

	bool fst = true;

	for (const char *s : lst) {
		if (fst)
			fst = false;
		else
			ImGui::TableNextColumn();

		ImGui::TextUnformatted(s);
	}

	ImGui::PopID();
}

Row::Row(int size, int id) : pos(0), size(size), id(id) {
	ImGui::PushID(id);
	ImGui::TableNextRow();
	ImGui::TableSetColumnIndex(0);
}

Row::~Row() {
	ImGui::PopID();
}

void Row::next() {
	if (pos < size) {
		++pos;
		ImGui::TableNextColumn();
	}
}

void Row::str(const std::string &s) {
	str(s.c_str());
}

void Row::str(const char *s) {
	assert(pos < size);

	ImGui::TextUnformatted(s);
	next();
}

void Row::strs(std::initializer_list<const char*> lst) {
	assert(lst.size() == size);

	for (const char *s : lst)
		str(s);

}

void Row::fmt(const char *s, ...) {
	va_list args;
	va_start(args, s);

	ImGui::TextV(s, args);

	va_end(args);

	next();
}

bool Row::chkbox(const char *s, bool &v) {
	bool b = ui::chkbox(s, v);
	next();
	return b;
}

bool Row::scalar(const char *label, int32_t &v, int32_t step) {
	bool b = ui::scalar(label, v, step);
	next();
	return b;
}

bool Row::scalar(const char *label, uint32_t &v, uint32_t step) {
	bool b = ui::scalar(label, v, step);
	next();
	return b;
}

bool Row::text(const char *label, std::string &buf, ImGuiInputTextFlags flags) {
	bool b = ui::text(label, buf, flags);
	next();
	return b;
}

HSVcol::HSVcol(int i) : i(i) {
	ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(i / 7.0f, 0.6f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(i / 7.0f, 0.7f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(i / 7.0f, 0.8f, 0.8f));
}

HSVcol::~HSVcol() {
	ImGui::PopStyleColor(3);
}

Popup::Popup(const std::string &description, PopupType type) : active(false), title(""), description(description), type(type) {
	switch (type) {
		case PopupType::error: title = "Error"; break;
		case PopupType::warning: title = "Warning"; break;
		default: title = "???"; break;
	}
}

bool Popup::show() {
	if (!active) {
		active = true;
		ImGui::OpenPopup(title.c_str());
	}

	ImGui::SetNextWindowSize(ImVec2(400, 0));

	if (ImGui::BeginPopupModal(title.c_str(), &active)) {
		ImGui::TextWrapped("%s", description.c_str());
		if (ImGui::Button("OK"))
			active = false;
		ImGui::EndPopup();
	}

	return active;
}

}

using namespace ui;

void Engine::show_chat_line(ui::Frame &f) {
	if (f.text("##", chat_line, ImGuiInputTextFlags_EnterReturnsTrue)) {
		if (chat_line == "/clear" || chat_line == "/cls") {
			chat.clear();
		} else {
			client->send_chat_text(chat_line);
			scroll_to_bottom = true;
		}

		chat_line.clear();
		ImGui::SetKeyboardFocusHere(-1);
	}
}

void Engine::show_mph_chat(ui::Frame &f) {
	Child cf;
	if (!cf.begin("ChatFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * (1 - player_height - frame_margin)), false, ImGuiWindowFlags_HorizontalScrollbar))
		return;

	if (f.text("Username", username, ImGuiInputTextFlags_EnterReturnsTrue))
		client->send_username(username);

	f.str("Chat");
	{
		Child ch;
		if (ch.begin("ChatHistory", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * 0.55f), true)) {
			for (std::string &s : chat)
				f.str(s.c_str());

			if (scroll_to_bottom) {
				scroll_to_bottom = false;
				ImGui::SetScrollHereY(1.0f);
			}
		}
	}

	show_chat_line(f);
}

void Engine::show_multiplayer_diplomacy() {
	Frame f;

	ImGuiIO &io = ImGui::GetIO();
	ImGui::SetNextWindowSizeConstraints(ImVec2(533, 380), ImVec2(io.DisplaySize.x, io.DisplaySize.y));

	if (!f.begin("Diplomacy", show_diplomacy))
		return;

	Assets &a = *assets.get();
	ImDrawList *lst = ImGui::GetWindowDrawList();

	const gfx::ImageRef &rbkg = a.at(io::DrsId::img_dialog0);
	ImVec2 pos(ImGui::GetWindowPos());
	ImVec2 tl(ImGui::GetWindowContentRegionMin()), br(ImGui::GetWindowContentRegionMax());
	tl.x += pos.x; tl.y += pos.y; br.x += pos.x; br.y += pos.y;

	for (float y = tl.y; y < br.y; y += rbkg.bnds.h) {
		float h = std::min<float>(rbkg.bnds.h, br.y - y);
		float t1 = rbkg.t0 + (rbkg.t1 - rbkg.t0) * h / rbkg.bnds.h;

		for (float x = tl.x; x < br.x; x += rbkg.bnds.w) {
			float w = std::min<float>(rbkg.bnds.w, br.x - x);
			float s1 = rbkg.s0 + (rbkg.s1 - rbkg.s0) * w / rbkg.bnds.w;

			lst->AddImage(tex1, ImVec2(x, y), ImVec2(x + w, y + h), ImVec2(rbkg.s0, rbkg.t0), ImVec2(s1, t1));
		}
	}

	{
		Table t;

		f.str("Work in progress...");

		if (t.begin("DiplomacyTable", 9)) {
			t.row(-1, { "Name", "Civilization", "Ally", "Neutral", "Enemy", "Food", "Wood", "Gold", "Stone" });

			for (unsigned i = 0; i < cv.scn.players.size(); ++i) {
				PlayerSetting &p = cv.scn.players[i];
				Row r(9, 0);

				ImGui::TextWrapped("%s", p.name.c_str());
				r.next();
				//r.str(p.name);
				r.str("oerkneuzen");
				r.chkbox("##0", show_diplomacy);
				r.chkbox("##1", show_diplomacy);
				r.chkbox("##2", show_diplomacy);
				r.next();
				r.next();
				r.next();
				r.next();
			}
		}
	}

	if (f.btn("OK")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		show_diplomacy = false;
	}

	f.sl();

	if (f.btn("Clear Tributes")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		// TODO clear fields
	}

	f.sl();

	if (f.btn("Cancel")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		show_diplomacy = false;
	}
}

void Engine::show_multiplayer_achievements() {
	ImGuiIO &io = ImGui::GetIO();

	float scale = io.DisplaySize.y / WINDOW_HEIGHT_MIN;

	ImVec2 max(std::min(io.DisplaySize.x, WINDOW_WIDTH_MAX + 8.0f), std::min(io.DisplaySize.y, WINDOW_HEIGHT_MAX + 8.0f));

	ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(max.x, 450 * scale), std::min(max.y, 420 * scale)), max);
	Frame f;

	if (!f.begin("Achievements", show_achievements))
		return;

	Assets &a = *assets.get();
	const gfx::ImageRef &ref = a.at(io::DrsId::bkg_achievements);

	ImDrawList *lst = ImGui::GetWindowDrawList();
	ImVec2 pos(ImGui::GetWindowPos());
	ImVec2 tl(ImGui::GetWindowContentRegionMin()), br(ImGui::GetWindowContentRegionMax());
	tl.x += pos.x; tl.y += pos.y; br.x += pos.x; br.y += pos.y;
	//ImVec2 pos(ImGui::GetWindowPos()), size(ImGui::GetWindowSize());
	//lst->AddImage(tex1, pos, ImVec2(pos.x + size.x, pos.y + size.y), ImVec2(ref.s0, ref.t0), ImVec2(ref.s1, ref.t1));
	lst->AddImage(tex1, tl, br, ImVec2(ref.s0, ref.t0), ImVec2(ref.s1, ref.t1));

	float w = br.x - tl.x;
	float sx = w / ref.bnds.w, sy = (br.y - tl.y) / ref.bnds.h;

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
			for (unsigned i = 0; i < gv.players.size(); ++i) {
				PlayerView &v = gv.players[i];

				float rowy = tl.y + 300 * sy + (348 - 300) * i * sy;

				std::string txt;

				txt = v.alive ? "Yes" : "No";

				sz = ImGui::CalcTextSize(txt.c_str());
				lst->AddText(ImVec2(tl.x + 662 * sx - sz.x / 2, rowy - sz.y), IM_COL32_WHITE, txt.c_str());

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
			show_achievements = false;
		}
	}
}

void Engine::show_terrain() {
	ZoneScoped;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *assets.get();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	const ImageSet &s_desert = a.anim_at(io::DrsId::trn_desert);
	const ImageSet &s_grass = a.anim_at(io::DrsId::trn_grass);
	const ImageSet &s_water = a.anim_at(io::DrsId::trn_water);
	const ImageSet &s_deepwater = a.anim_at(io::DrsId::trn_deepwater);

	const gfx::ImageRef &t0 = a.at(s_desert.imgs[0]);
	const gfx::ImageRef &t1 = a.at(s_grass.imgs[0]);
	const gfx::ImageRef &t2 = a.at(s_water.imgs[0]);
	const gfx::ImageRef &t3 = a.at(s_deepwater.imgs[0]);

	const gfx::ImageRef tt[] = { t0, t1, t2, t3 };

	float left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(cam_x) - 0.5f;
	float top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(cam_y) - 0.5f;

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

			ImVec2 tpos(tilepos(x, y, left, top, h));

			float x0 = tpos.x - img.hotspot_x;
			float y0 = tpos.y - img.hotspot_y;

			lst->AddImage(tex1, ImVec2(x0, y0), ImVec2(x0 + img.bnds.w, y0 + img.bnds.h), ImVec2(img.s0, img.t0), ImVec2(img.s1, img.t1), col);
		}
	}

	if (!io.WantCaptureMouse)
		ui.user_interact_entities();
	ui.show_buildings();
	ui.show_selections();
}

void UICache::show_editor_scenario() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *e->assets.get();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	if (!io.WantCaptureMouse) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		e->sdl->set_cursor(1);
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	}

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::MenuItem("Quit"))
			e->next_menu_state = MenuState::start;

		ImGui::EndMainMenuBar();
	}

	float menubar_bottom = vp->WorkPos.y;

	//50
	//625/768 -> 625-768=-143

	const gfx::ImageRef &bkg = a.at(io::DrsId::img_editor);

	float menubar_left = vp->WorkPos.x;

	// align center if menubar smaller than screen dimensions
	if (vp->WorkSize.x > bkg.bnds.w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - bkg.bnds.w) / 2;

	float t1, h = 50.0f;

	t1 = bkg.t0 + (bkg.t1 - bkg.t0) * h / bkg.bnds.h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_left + bkg.bnds.w, vp->WorkPos.y + h), ImVec2(bkg.s0, bkg.t0), ImVec2(bkg.s1, t1));

	h = 143.0f;
	float t0 = bkg.t0 + (bkg.t1 - bkg.t0) * (bkg.bnds.h - h) / bkg.bnds.h;

	lst->AddImage(e->tex1, ImVec2(menubar_left, vp->WorkPos.y + vp->WorkSize.y - h), ImVec2(menubar_left + bkg.bnds.w, vp->WorkPos.y + vp->WorkSize.y), ImVec2(bkg.s0, t0), ImVec2(bkg.s1, bkg.t1));
}

void Engine::show_multiplayer_game() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGuiIO &io = ImGui::GetIO();

	Assets &a = *assets.get();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	if (!io.WantCaptureMouse) {
		io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		sdl->set_cursor(1);
	} else {
		io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
	}

	float menubar_bottom = vp->WorkPos.y;

	const ImageSet &s = a.anim_at(io::DrsId::gif_menubar0);
	const gfx::ImageRef &rtop = a.at(s.imgs[0]), &rbottom = a.at(s.imgs[1]);

	float menubar_left = vp->WorkPos.x;

	// align center if menubar smaller than screen dimensions
	if (vp->WorkSize.x > rtop.bnds.w)
		menubar_left = vp->WorkPos.x + (vp->WorkSize.x - rtop.bnds.w) / 2;

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::Button("Chat")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			show_chat = !show_chat;
		}

		if (ImGui::Button("Diplomacy")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			show_diplomacy = !show_diplomacy;
		}

		// TODO refactor beginmenu/menuitem to our wrappers
		if (ImGui::BeginMenu("Menu")) {
			if (ImGui::MenuItem("Achievements"))
				show_achievements = !show_achievements;

			if (ImGui::MenuItem("Quit"))
				cancel_multiplayer_host(MenuState::defeat);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	int food = 200, wood = 200, gold = 0, stone = 150;
	std::string age(txt(StrId::age_stone));

	lst->AddImage(tex1, ImVec2(menubar_left, vp->WorkPos.y), ImVec2(menubar_left + rtop.bnds.w, vp->WorkPos.y + rtop.bnds.h), ImVec2(rtop.s0, rtop.t0), ImVec2(rtop.s1, rtop.t1));

	char buf[16];
	snprintf(buf, sizeof buf, "%d", food);
	buf[(sizeof buf) - 1] = '\0';

	float y = vp->WorkPos.y + 2;

	lst->AddText(ImVec2(menubar_left + 32 - 1, y + 1), IM_COL32(255, 255, 255, 255), buf);
	lst->AddText(ImVec2(menubar_left + 32, y), IM_COL32(0, 0, 0, 255), buf);

	snprintf(buf, sizeof buf, "%d", wood);
	buf[(sizeof buf) - 1] = '\0';

	lst->AddText(ImVec2(menubar_left + 99 - 1, y + 1), IM_COL32(255, 255, 255, 255), buf);
	lst->AddText(ImVec2(menubar_left + 99, y), IM_COL32(0, 0, 0, 255), buf);

	snprintf(buf, sizeof buf, "%d", gold);
	buf[(sizeof buf) - 1] = '\0';

	lst->AddText(ImVec2(menubar_left + 166 - 1, y + 1), IM_COL32(255, 255, 255, 255), buf);
	lst->AddText(ImVec2(menubar_left + 166, y), IM_COL32(0, 0, 0, 255), buf);

	snprintf(buf, sizeof buf, "%d", stone);
	buf[(sizeof buf) - 1] = '\0';

	lst->AddText(ImVec2(menubar_left + 234 - 1, y + 1), IM_COL32(255, 255, 255, 255), buf);
	lst->AddText(ImVec2(menubar_left + 234, y), IM_COL32(0, 0, 0, 255), buf);

	ImVec2 sz(ImGui::CalcTextSize(age.c_str()));

	lst->AddText(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) / 2, y + 1), IM_COL32(255, 255, 255, 255), age.c_str());
	lst->AddText(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) / 2, y), IM_COL32(0, 0, 0, 255), age.c_str());

	menubar_bottom += rtop.bnds.h;

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

	lst->AddImage(tex1, ImVec2(menubar_left, vp->WorkPos.y + vp->WorkSize.y - rbottom.bnds.h), ImVec2(menubar_left + rbottom.bnds.w, vp->WorkPos.y + vp->WorkSize.y), ImVec2(rbottom.s0, rbottom.t0), ImVec2(rbottom.s1, rbottom.t1));

	if (show_achievements)
		show_multiplayer_achievements();

	if (show_diplomacy)
		show_multiplayer_diplomacy();
}

bool Engine::locked_settings() const noexcept {
	return server.get() == nullptr || multiplayer_ready;
}

void Engine::show_multiplayer_host() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("multiplayer host", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	ScenarioSettings &scn = cv.scn;
	uint32_t player_count = scn.players.size();

	if (!locked_settings()) {
		f.str("Multiplayer game  -");
		f.sl();

		f.scalar(player_count == 1 ? "player" : "players", player_count, 1, 1, MAX_PLAYERS - 1);

		if (player_count != scn.players.size()) {
			client->send_players_resize(player_count);
			// resize immediately since we only need to send a message once
			scn.players.resize(player_count);
		}

		player_tbl_y = ImGui::GetCursorPosY();
	} else {
		f.fmt("Multiplayer game - %u %s", player_count, player_count == 1 ? "player" : "players");
	}

	if (player_tbl_y > 0)
		ImGui::SetCursorPosY(player_tbl_y);

	{
		Child lf;
		if (lf.begin("LeftFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.7f, ImGui::GetWindowHeight() * frame_height))) {
			Child pf;
			if (pf.begin("PlayerFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * player_height), false, ImGuiWindowFlags_HorizontalScrollbar))
				ui.show_mph_tbl(f);
		}
		show_mph_chat(f);

		f.chkbox("I'm Ready!", multiplayer_ready);

		f.sl();

		if (scn.players.empty() || !multiplayer_ready) {
			f.xbtn("Start Game");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
				if (scn.players.empty())
					ImGui::Tooltip("Game cannot be started without players. The host can add and remove players.");
				else
					ImGui::Tooltip("Click \"I'm Ready\" in order to start the game");
			}
		} else if (f.btn("Start Game")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			client->send_start_game();
		}

		f.sl();

		if (f.btn("Cancel")) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			cancel_multiplayer_host(MenuState::start);
		}
	}

	f.sl();
	{
		Child c;
		if (c.begin("SettingsFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.3f, ImGui::GetWindowHeight() * frame_height), false, ImGuiWindowFlags_HorizontalScrollbar))
			show_mph_cfg(f);
	}
}

void UICache::show_mph_tbl(ui::Frame &f) {
	Table t;

	unsigned idx = 0;
	ScenarioSettings &scn = e->cv.scn;

	auto it = scn.owners.find(e->cv.me);
	if (it != scn.owners.end())
		idx = it->second;

	if (t.begin("PlayerTable", 3)) {
		t.row(-1, {"Name", "Civ", "Team"});

		unsigned del = scn.players.size();
		unsigned from = 0, to = 0;

		for (unsigned i = 0; i < scn.players.size(); ++i) {
			Row r(3, i);
			PlayerSetting &p = scn.players[i];

			if (e->multiplayer_ready) {
				f.str(p.name);
				r.next();

				f.str(civs.at(p.civ));
				r.next();

				f.str(std::to_string(p.team));
				r.next();
				continue;
			}

			if (i + 1 == idx) {
				if (f.btn("X"))
					e->client->claim_player(0);

				f.sl();

				if (r.text("##0", p.name, ImGuiInputTextFlags_EnterReturnsTrue))
					e->client->send_set_player_name(i + 1, p.name);
			} else {
				if (f.btn("Claim"))
					e->client->claim_player(i + 1); // NOTE 1-based

				f.sl();

				if (!p.ai && e->server.get() != nullptr && f.btn("Set CPU"))
					e->client->claim_cpu(i + 1); // NOTE 1-based

				r.next();
			}

			if (f.combo("##1", p.civ, civs))
				e->client->send_set_player_civ(i + 1, p.civ);
			r.next();

			p.team = std::max(1u, p.team);
			if (f.scalar("##2", p.team, 1))
				e->client->send_set_player_team(i + 1, p.team);
			r.next();
		}
	}
}

void UICache::show_editor_menu() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("editor", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	float old_x = ImGui::GetCursorPosX();

	{
		FontGuard fg(e->fnt.fnt_copper2);

		ImGui::SetCursorPosY(38.0f / 768.0f * vp->WorkSize.y);
		f.str2("Scenario Editor", TextHalign::center);
	}

	FontGuard fg(e->fnt.fnt_copper);

	ImGui::SetCursorPosY(272.0f / 768.0f * vp->WorkSize.y);

	if (f.btn("Create Scenario", TextHalign::center)) {
		e->sfx.play_sfx(SfxId::sfx_ui_click);
		e->next_menu_state = MenuState::editor_scenario;
	}

	ImGui::SetCursorPosY(352.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Edit Scenario", TextHalign::center);

	ImGui::SetCursorPosY(432.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Campaign Editor", TextHalign::center);

	ImGui::SetCursorPosY(512.0f / 768.0f * vp->WorkSize.y);

	if (f.btn("Cancel", TextHalign::center)) {
		e->sfx.play_sfx(SfxId::sfx_ui_click);
		e->next_menu_state = MenuState::start;
	}

	ImGui::SetCursorPosX(old_x);
}


void UICache::load(Engine &e) {
	ZoneScoped;

	this->e = &e;
	civs.clear();
	e.assets->old_lang.collect_civs(civs);
}

void UICache::game_mouse_process() {
	ZoneScoped;

	ImGuiIO &io = ImGui::GetIO();

	if (io.WantCaptureMouse || !io.MouseDown[0] || io.MouseDownDuration[0] > 0.0f)
		return;

	ImGuiViewport *vp = ImGui::GetMainViewport();

	// click
	std::vector<std::pair<float, size_t>> selected;

	float off_x = io.MousePos.x, off_y = io.MousePos.y;

	// first pass to filter all rectangles
	for (size_t i = 0; i < entities.size(); ++i) {
		VisualEntity &v = entities[i];

		if (off_x >= v.x && off_x < v.x + v.w && off_y >= v.y && off_y < v.y + v.h)
			selected.emplace_back(v.z, i);
	}

	Assets &a = *e->assets.get();
	std::set<IdPoolRef> refs;

	// second pass to filter by silhouttes and to remove duplicates
	for (auto it = selected.begin(); it != selected.end();) {
		VisualEntity &v = entities[it->second];

		int y = (int)(off_y - v.y);

		const gfx::ImageRef &r = a.at(v.imgref);

		// ignore if out of range
		if (y < 0 || y >= r.mask.size()) {
			it = selected.erase(it);
			continue;
		}

		auto row = r.mask.at(y);
		int x = (int)(off_x - v.x);

		// ignore if point not on line
		if (x < row.first || x >= row.second) {
			it = selected.erase(it);
			continue;
		}

		// ignore if already added
		if (!refs.emplace(v.ref).second) {
			it = selected.erase(it);
			continue;
		}

		++it;
	}

	// sort on Z-order. NOTE: order is flipped compared to drawing
	std::sort(selected.begin(), selected.end(), [](const auto &lhs, const auto &rhs) { return lhs.first > rhs.first; });

	//printf("selected: %llu\n", (unsigned long long)selected.size());

	this->selected.clear();

	if (selected.empty())
		return;

	for (auto kv : selected) {
		VisualEntity &v = entities[kv.second];
		this->selected.emplace_back(v.ref);
	}

	if (this->selected.empty())
		return;

	// TODO inspect selected types
	IdPoolRef ref = *this->selected.begin();
	Entity *ent = e->gv.entities.try_get(ref);

	if (ent) {
		switch (ent->type) {
		case EntityType::town_center:
			e->sfx.play_sfx(SfxId::towncenter);
			break;
		case EntityType::barracks:
			e->sfx.play_sfx(SfxId::barracks);
			break;
		}
	}
}

void UICache::show_selections() {
	ZoneScoped;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	float left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(e->cam_x) - 0.5f;
	float top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(e->cam_y) - 0.5f;

	for (IdPoolRef ref : selected) {
		Entity *ent = e->gv.entities.try_get(ref);
		if (!ent)
			continue;

		// TODO determine entity (i.e. unit or building) size

		int x = ent->x, y = ent->y;
		uint8_t h = e->gv.t.h_at(x, y);

		ImVec2 tl(e->tilepos(x - 2, y - 1, left, top + e->th / 2, h));
		ImVec2 tb(e->tilepos(x + 1, y - 1, left, top + e->th / 2, h));
		ImVec2 tr(e->tilepos(x + 1, y + 1, left + e->tw / 2, top, h));
		ImVec2 tt(e->tilepos(x - 1, y + 1, left, top - e->th / 2, h));

		lst->AddLine(tl, tb, IM_COL32_WHITE);
		lst->AddLine(tb, tr, IM_COL32_WHITE);
		lst->AddLine(tr, tt, IM_COL32_WHITE);
		lst->AddLine(tt, tl, IM_COL32_WHITE);
	}
}

void UICache::show_buildings() {
	ZoneScoped;
	entities.clear();

	load_buildings();

	game_mouse_process();

	std::sort(entities.begin(), entities.end(), [](const VisualEntity &lhs, const VisualEntity &rhs){ return lhs.z < rhs.z; });
	
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	for (VisualEntity &v : entities) {
		lst->AddImage(e->tex1, ImVec2(v.x, v.y), ImVec2(v.x + v.w, v.y + v.h), ImVec2(v.s0, v.t0), ImVec2(v.s1, v.t1));
	}
}

void UICache::load_buildings() {
	ZoneScoped;
	
	ImGuiViewport *vp = ImGui::GetMainViewport();
	Assets &a = *e->assets.get();

	// TODO move to UICache::idle()
	float left = vp->WorkPos.x + vp->WorkSize.x / 2 - floor(e->cam_x) - 0.5f;
	float top = vp->WorkPos.y + vp->WorkSize.y / 2 - floor(e->cam_y) - 0.5f;

	for (auto kv : e->gv.entities) {
		Entity &ent = kv.second;

		io::DrsId bld_base = io::DrsId::bld_town_center;
		io::DrsId bld_player = io::DrsId::bld_town_center_player;

		switch (ent.type) {
		case EntityType::barracks:
			bld_base = bld_player = io::DrsId::bld_barracks;
			break;
		}

		int x = ent.x, y = ent.y;
		uint8_t h = e->gv.t.h_at(x, y);

		ImVec2 tpos(e->tilepos(x, y, left, top, h));
		float x0, y0;

		if (bld_player != bld_base) {
			const ImageSet &s_tc = a.anim_at(bld_base);
			IdPoolRef imgref = s_tc.imgs[0];
			const gfx::ImageRef &tc = a.at(imgref);

			x0 = tpos.x - tc.hotspot_x;
			y0 = tpos.y - tc.hotspot_y;

			entities.emplace_back(ent.ref, imgref, x0, y0, tc.bnds.w, tc.bnds.h, tc.s0, tc.t0, tc.s1, tc.t1, tpos.y);
		}

		const ImageSet &s_tcp = a.anim_at(bld_player);
		IdPoolRef imgref = s_tcp.at(ent.color, 0);
		const gfx::ImageRef &tcp = a.at(imgref);

		x0 = tpos.x - tcp.hotspot_x;
		y0 = tpos.y - tcp.hotspot_y;

		entities.emplace_back(ent.ref, imgref, x0, y0, tcp.bnds.w, tcp.bnds.h, tcp.s0, tcp.t0, tcp.s1, tcp.t1, tpos.y + 0.1f);
	}
}

void Engine::show_mph_cfg(ui::Frame &f) {
	ZoneScoped;
	ScenarioSettings &scn = cv.scn;

	f.str("Scenario: Random map");
	if (!server.get() || multiplayer_ready) {
		f.fmt("seed: %u", scn.seed);
		f.fmt("size: %ux%u", scn.width, scn.height);

		ImGui::BeginDisabled();
		f.chkbox("Wrap", scn.wrap);
		f.chkbox("Fixed position", scn.fixed_start);
		f.chkbox("Reveal map", scn.explored);
		f.chkbox("Full Tech Tree", scn.all_technologies);
		f.chkbox("Enable cheating", scn.cheating);
		ImGui::EndDisabled();

		f.fmt("Age: %u", scn.age);
		f.fmt("Max pop.: %u", scn.popcap);

		f.str("Resources:");
		f.fmt("food: %u", scn.res.food);
		f.fmt("wood: %u", scn.res.wood);
		f.fmt("gold: %u", scn.res.gold);
		f.fmt("stone: %u", scn.res.stone);

		f.fmt("villagers: %u", scn.villagers);
	} else {
		bool changed = false;

		changed |= f.scalar("seed", scn.seed);

		f.str("Size:");
		f.sl();
		if (changed |= f.chkbox("squared", scn.square) && scn.square)
			scn.width = scn.height;

		f.sl();
		changed |= f.chkbox("wrap", scn.wrap);

		if (changed |= f.scalar("Width", scn.width, 1, 8, 65536) && scn.square)
			scn.height = scn.width;
		if (changed |= f.scalar("Height", scn.height, 1, 8, 65536) && scn.square)
			scn.width = scn.height;

		changed |= f.chkbox("Fixed position", scn.fixed_start);
		changed |= f.chkbox("Reveal map", scn.explored);
		changed |= f.chkbox("Full Tech Tree", scn.all_technologies);
		changed |= f.chkbox("Enable cheating", scn.cheating);
		//if (scn.hosting)
		//	changed |= f.chkbox("Host makes settings", scn.restricted);

		changed |= f.scalar("Age", scn.age, 1, 1, 4);
		changed |= f.scalar("Max pop.", scn.popcap, 50, 5, 1000);

		f.str("Resources:");
		changed |= f.scalar("food", scn.res.food, 10);
		changed |= f.scalar("wood", scn.res.wood, 10);
		changed |= f.scalar("gold", scn.res.gold, 10);
		changed |= f.scalar("stone", scn.res.stone, 10);

		changed |= f.scalar("villagers", scn.villagers, 1, 1, scn.popcap);

		if (changed)
			client->send_scn_vars(scn);
	}
}

void Engine::show_defeat() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("start", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	if (f.btn("Timeline")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		// TODO show timeline
	}

	f.sl();

	if (f.btn("Close")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		next_menu_state = MenuState::start;
	}
}

void Engine::show_start() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("start", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	float old_x = ImGui::GetCursorPosX();

	{
		FontGuard fg(fnt.fnt_copper);

		//ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
		ImGui::SetCursorPosY(284.0f / 768.0f * vp->WorkSize.y);

		f.xbtn("Single Player", TextHalign::center);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			FontGuard fg2(fnt.fnt_arial);
			ImGui::Tooltip("Single player has not been implemented yet. However, you can start a singleplayer game by hosting a multiplayer session. In this multiplayer session, just add computer players.");
		}

		//ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
		ImGui::SetCursorPosY(364.0f / 768.0f * vp->WorkSize.y);

		if (f.btn("Multiplayer", TextHalign::center)) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			next_menu_state = MenuState::multiplayer_menu;
		}

		//ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
		ImGui::SetCursorPosY(444.0f / 768.0f * vp->WorkSize.y);

		f.xbtn("Help", TextHalign::center);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			FontGuard fg2(fnt.fnt_arial);
			ImGui::Tooltip("The original help is using a subsystem on Windows that has been removed in Windows 8. Consult the Github page or an Age of Empires forum if you need help.");
		}

		//ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
		ImGui::SetCursorPosY(524.0f / 768.0f * vp->WorkSize.y);

		if (f.btn("Scenario Builder", TextHalign::center)) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			next_menu_state = MenuState::editor_menu;
		}

		//ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
		ImGui::SetCursorPosY(604.0f / 768.0f * vp->WorkSize.y);

		if (f.btn("Exit", TextHalign::center)) {
			sfx.play_sfx(SfxId::sfx_ui_click);
			throw 0;
		}
	}

	ImGui::SetCursorPosX(old_x);
	ImGui::SetCursorPosY((710.0f - 30.0f) / 768.0f * vp->WorkSize.y);

	f.txt2(StrId::main_copy1, TextHalign::center);
	f.txt2(StrId::main_copy2b, TextHalign::center);
	//f.txt2(StrId::main_copy3, TextHalign::center);
	f.str2("Trademark reserved by Microsoft. Remake by Folkert van Verseveld.", TextHalign::center);
	//ImGui::TextWrapped("%s", "Copyright Age of Empires by Microsoft. Trademark reserved by Microsoft. Remake by Folkert van Verseveld");
}

static const std::vector<std::string> music_ids{ "menu", "success", "fail", "game" };

void Engine::show_init() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("init", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	f.str("Age of Empires game setup");

	ImGui::TextWrapped("%s", "In this menu, you can change general settings how the game behaves and where the game assets will be loaded from.");

	ImGui::TextWrapped("%s", "This game is free software. If you have paid for this free software remake, you have been scammed! If you like Age of Empires, please support Microsoft by buying the original game on Steam");


	if (!fnt.loaded()) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::TextWrapped("%s", "Cannot continue setup. Game fonts are not installed. Reinstall the game from CD-ROM and restart the setup. If you don't have a CD-ROM drive, copy the game directory and game fonts from a machine where the original game has been installed. If you don't own the game, you can buy it on Amazon or your favorite online retailer. The steam edition is not supported!");
		ImGui::PopStyleColor();

		ui::HSVcol col(0);

		if (f.btn("Quit"))
			throw 0;

		return;
	}

	if (assets_good) {
		if (f.btn("Start"))
			next_menu_state = MenuState::start;
	} else {
		f.xbtn("Start");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::Tooltip("Game directory must be set before the game can be run.");
	}

	if (f.btn("Quit"))
		throw 0;

	if (f.btn("Set game directory"))
		fd2.Open();

	fd2.Display();

	if (fd2.HasSelected()) {
		std::string path(fd2.GetSelected().string());
		verify_game_data(path);
		fd2.ClearSelected();
	}

	f.combo("music id", music_id, music_ids);

	if (f.btn("Open music"))
		fd.Open();

	fd.Display();

	if (fd.HasSelected()) {
		std::string path(fd.GetSelected().string());
		printf("selected \"%s\"\n", path.c_str());

		MusicId id = (MusicId)music_id;
		sfx.jukebox[id] = path;
		sfx.play_music(id);

		fd.ClearSelected();
	}

	f.sl();

	if (f.btn("Play"))
		sfx.play_music((MusicId)music_id);

	f.sl();

	if (f.btn("Stop"))
		sfx.stop_music();

	show_general_settings();

	ImGui::TextWrapped("%s", "Copyright Age of Empires by Microsoft. Trademark reserved by Microsoft. Remake by Folkert van Verseveld");
}

static const char *connection_modes[] = { "host game", "join game" };

void Engine::show_multiplayer_menu() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("multiplayer menu", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	float old_x = ImGui::GetCursorPosX();

	{
		FontGuard fg(fnt.fnt_copper2);

		ImGui::SetCursorPosY(38.0f / 768.0f * vp->WorkSize.y);
		f.str2("Multiplayer", TextHalign::center);
	}

	//f.text("username", username);

	if (ImGui::Combo("connection mode", &connection_mode, connection_modes, IM_ARRAYSIZE(connection_modes)))
		sfx.play_sfx(SfxId::sfx_ui_click);

	if (connection_mode == 1) {
		ImGui::InputText("host", connection_host, sizeof(connection_host));
		ImGui::SameLine();
		if (ImGui::Button("localhost"))
			strncpy0(connection_host, "127.0.0.1", sizeof(connection_host));
	}

	ImGui::InputScalar("port", ImGuiDataType_U16, &connection_port);

	if (f.btn("start")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		switch (connection_mode) {
			case 0:
				start_server(connection_port);
				break;
			case 1:
				start_client(connection_host, connection_port);
				break;
		}
	}

	ImGui::SameLine();

	if (f.btn("cancel")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		next_menu_state = MenuState::start;
	}

	ImGui::SetCursorPosX(old_x);
}

void Engine::draw_background_border() {
	ZoneScoped;
	assert(assets.get());
	Assets &a = *assets.get();

	BackgroundColors col;

	switch (menu_state) {
	case MenuState::multiplayer_host:
	case MenuState::multiplayer_menu:
		col = a.bkg_cols.at(io::DrsId::bkg_multiplayer);
		break;
	case MenuState::defeat:
		col = a.bkg_cols.at(io::DrsId::bkg_defeat);
		break;
	case MenuState::editor_menu:
	case MenuState::editor_scenario:
		col = a.bkg_cols.at(io::DrsId::bkg_editor_menu);
		break;
	default:
		col = a.bkg_cols.at(io::DrsId::bkg_main_menu);
		break;
	}

	ImDrawList *lst = ImGui::GetBackgroundDrawList();
	ImGuiIO &io = ImGui::GetIO();

	GLfloat right = io.DisplaySize.x, bottom = io.DisplaySize.y;

	lst->AddLine(ImVec2(0, 0), ImVec2(right, 0), IM_COL32(col.border[0].r, col.border[0].g, col.border[0].b, SDL_ALPHA_OPAQUE), 1);
	lst->AddLine(ImVec2(right - 1, 0), ImVec2(right - 1, bottom - 1), IM_COL32(col.border[0].r, col.border[0].g, col.border[0].b, SDL_ALPHA_OPAQUE), 1);

	lst->AddLine(ImVec2(1, 1), ImVec2(right - 1, 1), IM_COL32(col.border[1].r, col.border[1].g, col.border[1].b, SDL_ALPHA_OPAQUE), 1);
	lst->AddLine(ImVec2(right - 2, 1), ImVec2(right - 2, bottom - 2), IM_COL32(col.border[1].r, col.border[1].g, col.border[1].b, SDL_ALPHA_OPAQUE), 1);

	lst->AddLine(ImVec2(2, 2), ImVec2(right - 2, 2), IM_COL32(col.border[2].r, col.border[2].g, col.border[2].b, SDL_ALPHA_OPAQUE), 1);
	lst->AddLine(ImVec2(right - 3, 2), ImVec2(right - 3, bottom - 3), IM_COL32(col.border[2].r, col.border[2].g, col.border[2].b, SDL_ALPHA_OPAQUE), 1);

	lst->AddLine(ImVec2(0, 0), ImVec2(0, bottom), IM_COL32(col.border[5].r, col.border[5].g, col.border[5].b, SDL_ALPHA_OPAQUE), 1);
	lst->AddLine(ImVec2(0, bottom - 1), ImVec2(right, bottom - 1), IM_COL32(col.border[5].r, col.border[5].g, col.border[5].b, SDL_ALPHA_OPAQUE), 1);

	lst->AddLine(ImVec2(1, 1), ImVec2(1, bottom - 1), IM_COL32(col.border[4].r, col.border[4].g, col.border[4].b, SDL_ALPHA_OPAQUE), 1);
	lst->AddLine(ImVec2(1, bottom - 2), ImVec2(right - 1, bottom - 2), IM_COL32(col.border[4].r, col.border[4].g, col.border[4].b, SDL_ALPHA_OPAQUE), 1);

	lst->AddLine(ImVec2(2, 2), ImVec2(2, bottom - 2), IM_COL32(col.border[3].r, col.border[3].g, col.border[3].b, SDL_ALPHA_OPAQUE), 1);
	lst->AddLine(ImVec2(2, bottom - 3), ImVec2(right - 2, bottom - 3), IM_COL32(col.border[3].r, col.border[3].g, col.border[3].b, SDL_ALPHA_OPAQUE), 1);
}

}
