#include "engine.hpp"

#include "engine/sdl.hpp"

#include <cassert>
#include <cstdarg>
#include <cmath>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <minmax.hpp>
#include "ui.hpp"
#include "string.hpp"
#include "world/entity_info.hpp"
#include "world/terrain.hpp"

#include "../legacy/strings.hpp"

#if _WIN32
#include <shellapi.h>
#endif

namespace aoe {

namespace ui {

static inline bool sfx_process(bool v, Audio &sfx) {
	if (v)
		sfx.play_sfx(SfxId::ui_click);
	return v;
}

bool btn(Frame &f, const char *str, Audio &sfx) {
	return sfx_process(f.btn(str), sfx);
}

bool btn(Frame &f, StrId id, Audio &sfx) {
	return btn(f, old_lang.find(id).c_str(), sfx);
}

bool btn(Frame &f, const char *str, TextHalign ha, Audio &sfx) {
	return sfx_process(f.btn(str, ha), sfx);
}

bool chkbox(Frame &f, const char *str, bool &b, Audio &sfx) {
	return sfx_process(f.chkbox(str, b), sfx);
}

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
	return xbtn(s, NULL, ha, sz);
}

bool Frame::xbtn(const char *s, const char *tooltip, TextHalign ha, const ImVec2 &sz) {
	ImGui::BeginDisabled();
	bool b = btn(s, ha, sz);
	ImGui::EndDisabled();
	if (tooltip && *tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
		FontGuard fg2(fnt.arial);
		ImGui::Tooltip(tooltip);
	}
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

bool Frame::scalar(const char *label, int32_t &v, int32_t step, int32_t min, int32_t max) {
	v = std::clamp(v, min, max);
	int32_t v_old = v;
	bool b = ui::scalar(label, v, step);
	v = std::clamp(v, min, max);
	return b && v != v_old;
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

	if (multiplayer_ready) {
		ImGui::BeginDisabled();
		f.text("Username", username);
		ImGui::EndDisabled();
	} else if (f.text("Username", username, ImGuiInputTextFlags_EnterReturnsTrue)) {
		client->send_username(username);
	}

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

	if (btn(f, "OK", sfx))
		show_diplomacy = false;

	f.sl();

	if (btn(f, "Clear Tributes", sfx)) {
		// TODO clear fields
	}

	f.sl();

	if (btn(f, "Cancel", sfx))
		show_diplomacy = false;
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
		f.str("Multiplayer game -");
		f.sl();

		--player_count;
		f.scalar(player_count == 1 ? "player" : "players", player_count, 1, 1, MAX_PLAYERS - 1);
		++player_count;

		if (player_count != scn.players.size()) {
			client->send_players_resize(player_count);
			// resize immediately since we only need to send a message once
			scn.players.resize(player_count);
		}

		player_tbl_y = ImGui::GetCursorPosY();
	} else {
		--player_count;
		f.fmt("Multiplayer game - %u %s", player_count, player_count == 1 ? "player" : "players");
		++player_count;
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

		if (btn(f, "Cancel", sfx))
			quit_game(MenuState::start);

		if (server.get()) {
			f.sl();

			if (scn.players.empty() || !multiplayer_ready) {
				f.xbtn("Start Game");
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
					if (scn.players.empty())
						ImGui::Tooltip("Game cannot be started without players. The host can add and remove players.");
					else
						ImGui::Tooltip("Click \"I'm Ready\" in order to start the game");
				}
			} else if (btn(f, "Start Game", sfx)) {
				client->send_start_game();
			}
		}

		f.sl();

		if (f.chkbox("I'm Ready!", multiplayer_ready))
			client->send_ready(multiplayer_ready);
	}

	f.sl();
	{
		Child c;
		if (c.begin("SettingsFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.3f, ImGui::GetWindowHeight() * frame_height), false, ImGuiWindowFlags_HorizontalScrollbar))
			show_mph_cfg(f);
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
		FontGuard fg(fnt.copper2);

		ImGui::SetCursorPosY(38.0f / 768.0f * vp->WorkSize.y);
		f.str2("Scenario Editor", TextHalign::center);
	}

	FontGuard fg(fnt.copper);

	ImGui::SetCursorPosY(272.0f / 768.0f * vp->WorkSize.y);

	if (btn(f, "Create Scenario", TextHalign::center, e->sfx))
		e->next_menu_state = MenuState::editor_scenario;

	ImGui::SetCursorPosY(352.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Edit Scenario", TextHalign::center);

	ImGui::SetCursorPosY(432.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Campaign Editor", TextHalign::center);

	ImGui::SetCursorPosY(512.0f / 768.0f * vp->WorkSize.y);

	if (btn(f, "Cancel", TextHalign::center, e->sfx))
		e->next_menu_state = MenuState::start;

	ImGui::SetCursorPosX(old_x);
}


void UICache::load() {
	ZoneScoped;

	old_lang.collect_civs(civs);

	Assets &a = *this->e->assets.get();
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_desert));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_desert));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_grass));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_water));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_deepwater));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_desert_overlay));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_water_desert));
	t_imgs.emplace_back(a.anim_at(io::DrsId::trn_water_overlay));
}

void UICache::str2(const ImVec2 &pos, const char *text, bool invert) {
	ImU32 bg, fg;

	if (invert) {
		bg = IM_COL32(0, 0, 0, 255);
		fg = IM_COL32(255, 255, 255, 255);
	} else {
		bg = IM_COL32(255, 255, 255, 255);
		fg = IM_COL32(0, 0, 0, 255);
	}

	bkg->AddText(ImVec2(pos.x - 1, pos.y + 1), bg, text);
	bkg->AddText(pos, fg, text);
}

void UICache::strnum(const ImVec2 &pos, int v) {
	char buf[16];
	snprintf(buf, sizeof buf, "%d", v);
	str2(pos, buf);
}

void UICache::str_scream(const char *txt) {
	ImGuiIO &io = ImGui::GetIO();
	FontGuard fg(fnt.copper2);

	ImVec2 sz(ImGui::CalcTextSize(txt));

	bkg->AddText(ImVec2((io.DisplaySize.x - sz.x) / 2, (io.DisplaySize.y - sz.y) / 2), IM_COL32_WHITE, txt);
}

void UICache::game_mouse_process() {
	ZoneScoped;

	ImGuiIO &io = ImGui::GetIO();

	if (io.WantCaptureMouse)
		return;

	mouse_left_process();
	mouse_right_process();
}

void UICache::collect(std::vector<IdPoolRef> &dst, const SDL_Rect &area, bool filter) {
	dst.clear();

	// click
	std::vector<std::pair<float, size_t>> selected;

	// first pass to filter all rectangles
	for (size_t i = 0; i < entities.size(); ++i) {
		VisualEntity &v = entities[i];

		float mid_x = v.x + v.w * 0.5f, mid_y = v.y + v.h * 0.5f;

		if (mid_x >= area.x && mid_x < area.x + area.w && mid_y >= area.y && mid_y < area.y + area.h)
			selected.emplace_back(v.z, i);
	}

	Assets &a = *e->assets.get();
	std::set<IdPoolRef> refs;

	// second pass to remove duplicates
	for (auto it = selected.begin(); it != selected.end();) {
		VisualEntity &v = entities[it->second];

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

	// store entities and filter invalid and dead ones
	for (auto kv : selected) {
		VisualEntity &v = entities[kv.second];

		if (filter) {
			Entity *ent = e->gv.try_get(v.ref);
			if (ent && ent->is_alive())
				dst.emplace_back(v.ref);
		} else {
			dst.emplace_back(v.ref);
		}
	}
}

void UICache::collect(std::vector<IdPoolRef> &dst, float off_x, float off_y, bool filter) {
	dst.clear();

	// click
	std::vector<std::pair<float, size_t>> selected;

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
		if (v.xflip) {
			if (x < v.w - row.second || x >= v.w - row.first) {
				it = selected.erase(it);
				continue;
			}
		} else if (x < row.first || x >= row.second) {
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

	// store entities and filter invalid and dead ones
	for (auto kv : selected) {
		VisualEntity &v = entities[kv.second];

		if (filter) {
			Entity *ent = e->gv.try_get(v.ref);
			if (ent  && ent->is_alive())
				dst.emplace_back(v.ref);
		} else {
			dst.emplace_back(v.ref);
		}
	}
}

const gfx::ImageRef &UICache::imgtile(uint8_t id) {
	TileType type = Terrain::tile_type(id);
	unsigned subimage = Terrain::tile_img(id);

	Assets &a = *e->assets.get();
	return a.at(t_imgs.at((unsigned)type).imgs.at(subimage));
}

void UICache::show_selections() {
	ZoneScoped;

	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	for (IdPoolRef ref : selected) {
		Entity *ent = e->gv.try_get(ref);
		if (!ent)
			continue;

		// determine entity (i.e. unit or building) size
		const EntityInfo &i = entity_info.at((unsigned)ent->type);
		float size = i.size;

		float x = ent->x + 1, y = ent->y;
		int ix = ent->x + 1, iy = ent->y;
		uint8_t h = e->gv.t.h_at(ix, iy);

		ImVec2 tl(e->tilepos(x - 2, y - 1, left, top + e->th / 2, h));
		ImVec2 tb(e->tilepos(x + 1, y - 1, left, top + e->th / 2, h));
		ImVec2 tr(e->tilepos(x + 1, y + 1, left + e->tw / 2, top, h));
		ImVec2 tt(e->tilepos(x - 1, y + 1, left, top - e->th / 2, h));

		if (!is_building(ent->type)) {
			float offx = left, offy = top;

			tl = ImVec2(e->tilepos(x, y, offx - 2 * size, offy, h));
			tb = ImVec2(e->tilepos(x, y, offx, offy + size, h));
			tr = ImVec2(e->tilepos(x, y, offx + 2 * size, offy, h));
			tt = ImVec2(e->tilepos(x, y, offx, offy - size, h));
		}

		lst->AddLine(tl, tb, IM_COL32_WHITE);
		lst->AddLine(tb, tr, IM_COL32_WHITE);
		lst->AddLine(tr, tt, IM_COL32(255, 255, 255, 160));
		lst->AddLine(tt, tl, IM_COL32(255, 255, 255, 160));
	}

	if (multi_select) {
		ImGuiIO &io = ImGui::GetIO();

		lst->AddRect(
			ImVec2(std::min(start_x, io.MousePos.x), std::min(start_y, io.MousePos.y)),
			ImVec2(std::max(start_x, io.MousePos.x), std::max(start_y, io.MousePos.y)), IM_COL32_WHITE
		);
	}
}

void UICache::show_entities() {
	ZoneScoped;
	entities.clear();
	entities_deceased.clear();

	load_entities();

	game_mouse_process();

	std::sort(entities_deceased.begin(), entities_deceased.end(), [](const VisualEntity &lhs, const VisualEntity &rhs){ return lhs.z < rhs.z; });
	std::sort(entities.begin(), entities.end(), [](const VisualEntity &lhs, const VisualEntity &rhs){ return lhs.z < rhs.z; });
	
	ImDrawList *lst = ImGui::GetBackgroundDrawList();

	// push deceased entities first to make sure they are always below any other entities
	for (VisualEntity &v : entities_deceased) {
		lst->AddImage(e->tex1, ImVec2((int)v.x, (int)v.y), ImVec2(int(v.x + v.w), int(v.y + v.h)), ImVec2(v.s0, v.t0), ImVec2(v.s1, v.t1));
	}

	for (VisualEntity &v : entities) {
		lst->AddImage(e->tex1, ImVec2((int)v.x, (int)v.y), ImVec2(int(v.x + v.w), int(v.y + v.h)), ImVec2(v.s0, v.t0), ImVec2(v.s1, v.t1));
	}
}

#define MAPSIZE_STEP 4

// TODO extract to ui/widgets/scenario_settings.cpp
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
		if (changed |= f.chkbox("square", scn.square) && scn.square)
			scn.width = scn.height;

		f.sl();
		changed |= f.chkbox("wrap around", scn.wrap);

		if (changed |= f.scalar("Width", scn.width, MAPSIZE_STEP, Terrain::min_size, Terrain::max_size) && scn.square)
			scn.height = scn.width;
		if (changed |= f.scalar("Height", scn.height, MAPSIZE_STEP, Terrain::min_size, Terrain::max_size) && scn.square)
			scn.width = scn.height;

		changed |= f.chkbox("Fixed position", scn.fixed_start);
		changed |= f.chkbox("Reveal map", scn.explored);
		changed |= f.chkbox("Full Tech Tree", scn.all_technologies);
		changed |= f.chkbox("Enable cheating", scn.cheating);
		//if (scn.hosting)
		//	changed |= f.chkbox("Host makes settings", scn.restricted);

		changed |= f.combo("Age", scn.age, old_lang.age_names);
		changed |= f.scalar("Max pop.", scn.popcap, 50, 5, 1000);

		f.str("Resources:");
		changed |= f.scalar("food", scn.res.food, 10);
		changed |= f.scalar("wood", scn.res.wood, 10);
		changed |= f.scalar("gold", scn.res.gold, 10);
		changed |= f.scalar("stone", scn.res.stone, 10);

		changed |= f.scalar("villagers", scn.villagers, 1, ScenarioSettings::min_villagers, std::min(scn.popcap, ScenarioSettings::max_villagers));

		if (changed)
			client->send_scn_vars(scn);
	}
}

void Engine::show_gameover() {
	ZoneScoped;
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowSize(vp->WorkSize);
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("start", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	if (!show_achievements(f))
		next_menu_state = MenuState::start;
}

void Engine::open_help() {
	ZoneScoped;
#if _WIN32
	sdl->window.set_fullscreen(false);
	ShellExecute(NULL, NULL, "https://github.com/FolkertVanVerseveld/aoe", NULL, NULL, SW_SHOW);
#endif
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
		FontGuard fg(fnt.copper);

		ImGui::SetCursorPosY(284.0f / 768.0f * vp->WorkSize.y);

#if 0
		if (btn(f, "Single Player", TextHalign::center, sfx))
			next_menu_state = MenuState::singleplayer_menu;
#else
		f.xbtn("Single Player", "Single player mode is not supported yet. Use multiplayer mode with 1 player instead.", TextHalign::center);
#endif

		ImGui::SetCursorPosY(364.0f / 768.0f * vp->WorkSize.y);

		if (btn(f, "Multiplayer", TextHalign::center, sfx))
			next_menu_state = MenuState::multiplayer_menu;

		ImGui::SetCursorPosY(444.0f / 768.0f * vp->WorkSize.y);
#if _WIN32
		if (btn(f, "Help", TextHalign::center, sfx))
			open_help();
#else
		f.xbtn("Help", "The original help is using a subsystem on Windows that is not supported. Consult the Github page or an Age of Empires forum if you need help.", TextHalign::center);
#endif

		ImGui::SetCursorPosY(524.0f / 768.0f * vp->WorkSize.y);

		if (btn(f, "Scenario Builder", TextHalign::center, sfx))
			next_menu_state = MenuState::editor_menu;

		ImGui::SetCursorPosY(604.0f / 768.0f * vp->WorkSize.y);

		if (btn(f, "Exit", TextHalign::center, sfx))
			throw 0;
	}

	ImGui::SetCursorPosX(old_x);
	ImGui::SetCursorPosY((710.0f - 40.0f) / 768.0f * vp->WorkSize.y);

	f.str2("Trademark reserved by Microsoft. Remake by Folkert van Verseveld.", TextHalign::center);
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
		if (f.btn("Start")) {
			next_menu_state = MenuState::start;
			cfg.game_dir = game_dir;
			cfg.save("config.ini");
		}
	} else {
		f.xbtn("Start", "Game directory must be set before the game can be run.");
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

void Engine::multiplayer_set_localhost() {
	strncpy0(connection_host, "127.0.0.1", sizeof(connection_host));
}

#define SetRelY(ry) ImGui::SetCursorPosY((ry) / 768.0f * vp->WorkSize.y)

void Engine::show_singleplayer_menu() {
	using namespace ImGui;
	ZoneScoped;
	ImGuiViewport *vp = GetMainViewport();
	SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("singleplayer menu", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	SetWindowSize(vp->WorkSize);

	float old_x = GetCursorPosX();

	{
		FontGuard fg(fnt.copper2);

		SetRelY(38.0f);
		f.str2("Single Player", TextHalign::center);
	}

	FontGuard fg(fnt.copper);

	SetRelY(188.0f);
	if (btn(f, "Random Map", TextHalign::center, sfx))
		next_menu_state = MenuState::singleplayer_host;

	SetRelY(268.0f);
	f.xbtn("Campaign", TextHalign::center);

	SetRelY(348.0f);
	if (btn(f, "Death Match", TextHalign::center, sfx))
		next_menu_state = MenuState::singleplayer_host;

	SetRelY(428);
	f.xbtn("Scenario", TextHalign::center);

	SetRelY(508.0f);
	f.xbtn("Saved Game", TextHalign::center);

	SetRelY(588.0f);
	if (btn(f, "Cancel", TextHalign::center, sfx))
		next_menu_state = MenuState::start;
}

void Engine::show_singleplayer_host() {
	using namespace ImGui;
	ZoneScoped;

	ImGuiViewport *vp = GetMainViewport();
	SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("singleplayer host", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	SetWindowSize(vp->WorkSize);

	{
		FontGuard fg(fnt.copper2);

		SetRelY(14.0f);
		f.str2("Single Player Game", TextHalign::center);
	}

	{
		Child lf;
		if (lf.begin("LeftFrame", ImVec2(GetWindowContentRegionWidth() * 0.7f, GetWindowHeight() * (frame_height - 0.05f)))) {
			Child pf;
			if (pf.begin("PlayerFrame", ImVec2(GetWindowContentRegionWidth(), GetWindowHeight() * player_height), false, ImGuiWindowFlags_HorizontalScrollbar))
				show_player_game_table(f);
		}
		SetRelY(515);
		f.scalar("Player count", sp_player_ui_count, 1, 1, max_legacy_players - 1);

		if (btn(f, "Randomize", sfx)) {
			sp_player_count = sp_player_ui_count + 1;
			sp_game_settings_randomize();
		}

		f.sl();
		chkbox(f, "teams as well", sp_randomize_teams, sfx);
	}

	f.sl();

	{
		Child c;
		if (c.begin("SettingsFrame", ImVec2(GetWindowContentRegionWidth() * 0.3f, GetWindowHeight() * (frame_height - 0.05f)), false, ImGuiWindowFlags_HorizontalScrollbar))
			show_scenario_settings(f, sp_scn);
	}

	if (btn(f, "Start Game", sfx)) {
		sp_player_count = sp_player_ui_count + 1;
		start_singleplayer_game();
	}

	f.sl();

	if (btn(f, StrId::btn_cancel, sfx))
		next_menu_state = MenuState::singleplayer_menu;
}

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
		FontGuard fg(fnt.copper2);

		SetRelY(38.0f);
		f.str2("Multiplayer", TextHalign::center);
	}

	f.text("Username", username, ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::RadioButton(connection_modes[0], &connection_mode, 0); ImGui::SameLine();
	ImGui::RadioButton(connection_modes[1], &connection_mode, 1);

	if (connection_mode == 1) {
		ImGui::InputText("host", connection_host, sizeof(connection_host));
		ImGui::SameLine();
		if (ImGui::Button("localhost"))
			multiplayer_set_localhost();
	}

	ImGui::InputScalar("port", ImGuiDataType_U16, &connection_port);

	if (isspace(username)) {
		const char *tooltip = "Enter a username before hosting a game";
		if (connection_mode)
			tooltip = "Enter a username before joining a game";

		f.xbtn("start", tooltip);
	} else if (btn(f, "start", sfx)) {
		switch (connection_mode) {
			case 0:
				start_server(connection_port);
				break;
			case 1:
				if (!connection_host[0])
					multiplayer_set_localhost();

				start_client(connection_host, connection_port);
				break;
		}
	}

	ImGui::SameLine();

	if (btn(f, "Cancel", sfx))
		next_menu_state = MenuState::start;

	ImGui::SetCursorPosX(old_x);
}

void DrawLine(ImDrawList *lst, float x0, float y0, float x1, float y1, SDL_Color col)
{
	lst->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(col.r, col.g, col.b, SDL_ALPHA_OPAQUE), 1);
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
	case MenuState::singleplayer_menu:
	case MenuState::singleplayer_host:
		col = a.bkg_cols.at(io::DrsId::bkg_singleplayer);
		break;
	case MenuState::defeat:
		col = a.bkg_cols.at(io::DrsId::bkg_defeat);
		break;
	case MenuState::victory:
		col = a.bkg_cols.at(io::DrsId::bkg_victory);
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

	DrawLine(lst, 0, 0, right, 0, col.border[0]);
	DrawLine(lst, right - 1, 0, right - 1, bottom - 1, col.border[0]);

	DrawLine(lst, 1, 1, right - 1, 1, col.border[1]);
	DrawLine(lst, right - 2, 1, right - 2, bottom - 2, col.border[1]);

	DrawLine(lst, 2, 2, right - 2, 2, col.border[2]);
	DrawLine(lst, right - 3, 2, right - 3, bottom - 3, col.border[2]);

	DrawLine(lst, 0, 0, 0, bottom, col.border[5]);
	DrawLine(lst, 0, bottom - 1, right, bottom - 1, col.border[5]);

	DrawLine(lst, 1, 1, 1, bottom - 1, col.border[4]);
	DrawLine(lst, 1, bottom - 2, right - 1, bottom - 2, col.border[4]);

	DrawLine(lst, 2, 2, 2, bottom - 2, col.border[3]);
	DrawLine(lst, 2, bottom - 3, right - 2, bottom - 3, col.border[3]);
}

}
