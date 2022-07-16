#include "engine.hpp"

#include "sdl.hpp"

#include <cassert>
#include <cstdarg>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include "nominmax.hpp"
#include "ui.hpp"

#include <algorithm>

namespace aoe {

const std::vector<std::string> civs{ "oerkneuzen", "sukkols", "medeas", "jasons" };

namespace ui {

Frame::Frame() : open(false) {}

Frame::~Frame() {
	if (open)
		close();
}

bool Frame::begin(const char *name, ImGuiWindowFlags flags) {
	assert(!open);
	open = ImGui::Begin(name, NULL, flags);
	return open;
}

bool Frame::begin(const char *name, bool &p_open, ImGuiWindowFlags flags) {
	assert(!open);
	open = ImGui::Begin(name, &p_open, flags);
	return open;
}

void Frame::close() {
	assert(open);
	ImGui::End();
	open = false;
}

void Frame::str(const char *s) {
	ImGui::TextUnformatted(s);
}

static bool chkbox(const char *s, bool &b) {
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

bool Frame::combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height) {
	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		bool b = ImGui::Combo(label, idx, lst, popup_max_height);
		ImGui::PopItemWidth();
		return b;
	}

	return ImGui::Combo(label, idx, lst, popup_max_height);
}

static bool scalar(const char *label, int32_t &v, int32_t step) {
	static_assert(sizeof(v) == sizeof(int32_t));

	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		bool b = ImGui::InputScalar(label, ImGuiDataType_S32, &v, step ? (const void *)&step : NULL);
		ImGui::PopItemWidth();
		return b;
	}

	return ImGui::InputScalar(label, ImGuiDataType_U32, &v, step ? (const void *)&step : NULL);
}

static bool scalar(const char *label, uint32_t &v, uint32_t step) {
	static_assert(sizeof(v) == sizeof(uint32_t));

	if (label && *label == '#') {
		ImGui::PushItemWidth(-1);
		bool b = ImGui::InputScalar(label, ImGuiDataType_U32, &v, step ? (const void *)&step : NULL);
		ImGui::PopItemWidth();
		return b;
	}

	return ImGui::InputScalar(label, ImGuiDataType_U32, &v, step ? (const void*)&step : NULL);
}

bool Frame::scalar(const char *label, int32_t &v, int32_t step) {
	return ui::scalar(label, v, step);
}

bool Frame::scalar(const char *label, uint32_t &v, uint32_t step) {
	return ui::scalar(label, v, step);
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

}

using namespace ui;

void Engine::show_multiplayer_host() {
	ImGuiViewport *vp = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(vp->WorkPos);

	Frame f;

	if (!f.begin("multiplayer host", ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse))
		return;

	ImGui::SetWindowSize(vp->WorkSize);

	f.str("Multiplayer game");

	float frame_height = 0.85f;
	float player_height = 0.7f;
	float frame_margin = 0.075f;

	ImGui::BeginChild("LeftFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.7f, ImGui::GetWindowHeight() * frame_height));
	ImGui::BeginChild("PlayerFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * player_height), false, ImGuiWindowFlags_HorizontalScrollbar);
	{
		Table t;

		if (t.begin("PlayerTable", 5)) {
			t.row(-1, {"Type", "Name", "Civ", "Player", "Team"});

			unsigned del = scn.players.size();
			unsigned from = 0, to = 0;

			for (unsigned i = 0; i < scn.players.size(); ++i) {
				Row r(5, i);
				PlayerSetting &p = scn.players[i];

				if (f.btn("X"))
					del = i;
				f.sl();
				if (f.btn("^")) {
					from = (i + scn.players.size() - 1) % scn.players.size();
					to = i;
				}
				f.sl();
				if (f.btn("V")) {
					from = i;
					to = (i + 1) % scn.players.size();
				}
				f.sl();
				r.chkbox("AI", p.ai);

				r.text("##0", p.name);

				f.combo("##1", p.civ, civs);
				r.next();

				p.index = std::max(1u, p.index);
				r.scalar("##2", p.index, 1);

				p.team = std::max(1u, p.team);
				r.scalar("##3", p.team, 1);
			}

			if (del >= 0 && del < scn.players.size())
				scn.players.erase(scn.players.begin() + del);

			if (from != to && !scn.players.empty()) {
				PlayerSetting tmp = scn.players[from];
				scn.players[from] = scn.players[to];
				scn.players[to] = tmp;
			}
		}
	}

	if (f.btn("+"))
		scn.players.emplace_back();

	ImGui::EndChild();
	ImGui::BeginChild("ChatFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * (1 - player_height - frame_margin)), false, ImGuiWindowFlags_HorizontalScrollbar);
	f.str("Chat");
	// TODO add chat
	f.text("##", chat_line);
	ImGui::EndChild();
	ImGui::EndChild();

	f.sl();
	ImGui::BeginChild("SettingsFrame", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.3f, ImGui::GetWindowHeight() * frame_height), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (ImGui::Button("Settings")) {
	}

	f.str("Scenario: Random map");

	f.scalar("Map width", scn.width);
	f.scalar("Map height", scn.height);

	f.chkbox("Fixed position", scn.fixed_start);
	f.chkbox("Reveal map", scn.explored);
	f.chkbox("Full Tech Tree", scn.all_technologies);
	f.chkbox("Enable cheating", scn.cheating);

	f.scalar("Population limit", scn.popcap);

	ImGui::EndChild();

	f.chkbox("I'm Ready!", multiplayer_ready);

	f.sl();

	if (scn.players.empty())
		f.xbtn("Start Game");
	else if (f.btn("Start Game"))
		menu_state = MenuState::multiplayer_game;

	f.sl();

	if (f.btn("Cancel"))
		menu_state = MenuState::init;
}

}
