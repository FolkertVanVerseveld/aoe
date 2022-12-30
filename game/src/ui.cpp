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

const std::vector<std::string> civs{ "oerkneuzen", "sukkols", "medeas", "jasons" };

namespace ui {

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

void Frame::str(const char *s) {
	ImGui::TextUnformatted(s);
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

bool Frame::btn(const char *s, const ImVec2 &sz) {
	return ImGui::Button(s, sz);
}

bool Frame::xbtn(const char *s, const ImVec2 &sz) {
	ImGui::BeginDisabled();
	bool b = ImGui::Button(s, sz);
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

			Row r(9, 0);

			r.str("You");
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
	ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(1024, 768));
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
			Table t;

			if (t.begin("SummaryTable", 8)) {
				t.row(-1, {" ", "Military", "Economy", "Religion", "Technology", "Survival", "Wonder", "Total Score"});

				Row r(8, 0);

				r.str("test");
				r.str("0");
				r.str("0");
				r.str("0");
				r.str("0");
				r.str("Yes");
				r.str("No");
				r.str("100");
			}
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

	const ImageSet &s_tc = a.anim_at(io::DrsId::bld_town_center);
	const gfx::ImageRef &tc = a.at(s_tc.imgs[0]);

	int x = 2, y = 1;
	uint8_t h = gv.t.h_at(x, y);

	ImVec2 tpos(tilepos(x, y, left, top, h));
	float x0, y0;

	x0 = tpos.x - tc.hotspot_x;
	y0 = tpos.y - tc.hotspot_y;

	lst->AddImage(tex1, ImVec2(x0, y0), ImVec2(x0 + tc.bnds.w, y0 + tc.bnds.h), ImVec2(tc.s0, tc.t0), ImVec2(tc.s1, tc.t1));

	const ImageSet &s_tcp = a.anim_at(io::DrsId::bld_town_center_player);
	const gfx::ImageRef &tcp = a.at(s_tcp.imgs[0]);

	x0 = tpos.x - tcp.hotspot_x;
	y0 = tpos.y - tcp.hotspot_y;

	lst->AddImage(tex1, ImVec2(x0, y0), ImVec2(x0 + tcp.bnds.w, y0 + tcp.bnds.h), ImVec2(tcp.s0, tcp.t0), ImVec2(tcp.s1, tcp.t1));
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
	const char *age = "Stone Age";

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

	ImVec2 sz(ImGui::CalcTextSize(age));

	lst->AddText(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) / 2, y + 1), IM_COL32(255, 255, 255, 255), age);
	lst->AddText(ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) / 2, y), IM_COL32(0, 0, 0, 255), age);

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

void Engine::show_multiplayer_host() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("multiplayer host", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	uint32_t player_count = scn.players.size();

	if (scn.is_enabled()) {
		f.str("Multiplayer game  -");
		f.sl();

		f.scalar(player_count == 1 ? "player" : "players", player_count, 1, 1, 256);

		if (player_count != scn.players.size()) {
			client->send_players_resize(player_count);
			// resize immediately since we only need to send a message once
			scn.players.resize(player_count);
		}
	} else {
		f.fmt("Multiplayer game - %u %s", player_count, player_count == 1 ? "player" : "players");
	}

	{
		Child lf;
		if (lf.begin("LeftFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.7f, ImGui::GetWindowHeight() * frame_height))) {
			Child pf;
			if (pf.begin("PlayerFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * player_height), false, ImGuiWindowFlags_HorizontalScrollbar))
				show_mph_tbl(f);
		}
		show_mph_chat(f);

		f.chkbox("I'm Ready!", multiplayer_ready);

		f.sl();

		if (scn.players.empty()) {
			f.xbtn("Start Game");
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
				ImGui::Tooltip("Game cannot be started without players. You can add players with `+' and `+10'.");
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

void Engine::show_mph_tbl(ui::Frame &f) {
	{
		Table t;

		if (t.begin("PlayerTable", 3)) {
			t.row(-1, {"Name", "Civ", "Team"});

			unsigned del = scn.players.size();
			unsigned from = 0, to = 0;

			for (unsigned i = 0; i < scn.players.size(); ++i) {
				Row r(3, i);
				PlayerSetting &p = scn.players[i];

				if (f.btn("X"))
					del = i;

				f.sl();

				//r.text("##0", p.name);

				if (f.btn("Claim"))
					;

				if (ImGui::IsItemHovered()) {
					ImGui::Tooltip("bla bla");
				}

				f.sl();

				if (f.btn("Set AI"))
					;

				r.next();

				f.combo("##1", p.civ, civs);
				r.next();

				p.team = std::max(1u, p.team);
				f.scalar("##2", p.team, 1);
				r.next();
			}

			if (del >= 0 && del < scn.players.size())
				scn.players.erase(scn.players.begin() + del);
		}
	}

	while (scn.players.size() >= MAX_PLAYERS)
		scn.players.pop_back();
}

void Engine::show_mph_cfg(ui::Frame &f) {
	f.str("Scenario: Random map");
	if (!scn.is_enabled()) {
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

	ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
	ImGui::SetCursorPosY(284.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Single Player");

	ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
	ImGui::SetCursorPosY(364.0f / 768.0f * vp->WorkSize.y);

	if (f.btn("Multiplayer")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		next_menu_state = MenuState::multiplayer_menu;
	}

	ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
	ImGui::SetCursorPosY(444.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Help");

	ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
	ImGui::SetCursorPosY(524.0f / 768.0f * vp->WorkSize.y);

	f.xbtn("Scenario Builder");

	ImGui::SetCursorPosX(429.0f / 1024.0f * vp->WorkSize.x);
	ImGui::SetCursorPosY(604.0f / 768.0f * vp->WorkSize.y);

	if (f.btn("Exit")) {
		sfx.play_sfx(SfxId::sfx_ui_click);
		throw 0;
	}

	ImGui::SetCursorPosX(old_x);
	ImGui::SetCursorPosY(710.0f / 768.0f * vp->WorkSize.y);

	ImGui::TextWrapped("%s", "Copyright Age of Empires by Microsoft. Trademark reserved by Microsoft. Remake by Folkert van Verseveld");
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
}

void Engine::draw_background_border() {
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
