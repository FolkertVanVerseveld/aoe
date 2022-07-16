#include "engine.hpp"

#include "sdl.hpp"

#include <cassert>
#include <cstdarg>

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include "ui.hpp"

namespace aoe {

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

bool Frame::chkbox(const char *s, bool &b) {
	return ImGui::Checkbox(s, &b);
}

bool Frame::btn(const char *s, const ImVec2 &sz) {
	return ImGui::Button(s, sz);
}

bool Frame::scalar(const char *label, uint32_t &v) {
	static_assert(sizeof(v) == sizeof(uint32_t));
	return ImGui::InputScalar(label, ImGuiDataType_U32, &v);
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

		if (t.begin("PlayerTable", 4)) {
			t.row(-1, {"Name", "Civ", "Player", "Team"});

			for (unsigned i = 0; i < 8; ++i) {
				Row r(4, i);

				r.fmt("player %u", i + 1);
				r.str("Macedonian");
				r.fmt("%u", i + 1);
				r.str("-");
			}
		}
	}

	ImGui::EndChild();
	ImGui::BeginChild("ChatFrame", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowHeight() * (1 - player_height - frame_margin)), false, ImGuiWindowFlags_HorizontalScrollbar);
	f.str("Chat");
	// TODO add chat
	ImGui::InputText("##", chat_line);
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

	if (f.btn("Start Game"))
		menu_state = MenuState::multiplayer_game;

	f.sl();

	if (f.btn("Cancel"))
		menu_state = MenuState::init;
}

}
