#pragma once

#include "imgui_user.hpp"

#include "imfilebrowser.h"

#include "idpool.hpp"
#include "legacy/strings.hpp"

#include <SDL2/SDL_rect.h>

#include <cstdint>

#include <initializer_list>
#include <vector>

/**
 * Wrappers to make ImGui:: calls less tedious
 */

namespace aoe {

class Engine;

namespace ui {

bool chkbox(const char *s, bool &b);
bool combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height_in_items=-1);

enum class TextHalign {
	left,
	center,
	right,
};

void str(const char*, TextHalign ha, bool wrap);

class Frame final {
	bool open, active;
public:
	Frame();
	~Frame();

	bool begin(const char *name, ImGuiWindowFlags flags=0);
	bool begin(const char *name, bool &p_open, ImGuiWindowFlags flags=0);
	void close();

	/** Like str but with a background color. */
	void str2(const std::string&, TextHalign ha=TextHalign::left, const ImVec4 &bg=ImVec4(0, 0, 0, 1), bool wrap=true);
	void str2(const char*, TextHalign ha=TextHalign::left, const ImVec4 &bg=ImVec4(0, 0, 0, 1), bool wrap=true);
	void txt2(StrId, TextHalign ha=TextHalign::left, const ImVec4 &bg = ImVec4(0, 0, 0, 1));

	void str(const std::string&);
	void str(const char*);
	void str(StrId);
	void fmt(const char*, ...);

	bool chkbox(const char*, bool&);
	bool btn(const char*, TextHalign ha=TextHalign::left, const ImVec2 &sz=ImVec2(0, 0));
	bool xbtn(const char*, TextHalign ha=TextHalign::left, const ImVec2 &sz=ImVec2(0, 0)); // like btn, but disabled
	bool combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height_in_items=-1);

	// typed input fields
	bool scalar(const char *label, int32_t &v, int32_t step=0);
	bool scalar(const char *label, uint32_t &v, uint32_t step=0);
	// like scalar but clamps the passed v
	bool scalar(const char *label, uint32_t &v, uint32_t step, uint32_t min, uint32_t max);
	bool text(const char *label, std::string &buf, ImGuiInputTextFlags flags=0);

	void sl(); // SameLine

	constexpr operator bool() const noexcept { return open; }
};

class Child final {
	bool open, active;
public:
	Child();
	~Child();

	bool begin(const char *str_id, const ImVec2 &size, bool border=false, ImGuiWindowFlags extra_flags=0);
	void close();

	constexpr operator bool() const noexcept { return open; }
};

class MenuBar final {
	bool open;
public:
	MenuBar();
	~MenuBar();

	void close();

	constexpr operator bool() const noexcept { return open; }
};

class Menu final {
	bool open;
public:
	Menu();
	~Menu();

	Menu(const Menu&) = delete;
	Menu(Menu&&) noexcept;

	bool begin(const char *str_id);
	void close();

	bool item(const char*);
	bool chkbox(const char*, bool&);
	bool btn(const char*, const ImVec2 &sz=ImVec2(0, 0));

	constexpr operator bool() const noexcept { return open; }
};

class Table final {
	bool open;
public:
	Table();
	~Table();

	bool begin(const char *str_id, int columns, ImGuiTableFlags flags=0);
	void close();

	void row(int id, std::initializer_list<const char*>);

	constexpr operator bool() const noexcept { return open; }
};

class Row final {
	int pos;
public:
	const int size, id;

	Row(int size, int id);
	~Row();

	void next();

	// similar to Frame's functions but these all automagically call next upon return
	void str(const std::string&);
	void str(const char*);
	void strs(std::initializer_list<const char*>);

	void fmt(const char*, ...);

	bool chkbox(const char*, bool&);

	// typed input fields
	bool scalar(const char *label, int32_t &v, int32_t step=0);
	bool scalar(const char *label, uint32_t &v, uint32_t step=0);
	bool text(const char *label, std::string &buf, ImGuiInputTextFlags flags=0);
};

class HSVcol {
	int i;
public:
	HSVcol(int i);
	~HSVcol();
};

enum class PopupType {
	error,
	warning,
	info,
};

class Popup final {
	bool active;
public:
	std::string title;
	std::string description;
	PopupType type;

	Popup(const std::string &description, PopupType type);

	bool show();
};

struct VisualEntity final {
	IdPoolRef ref, imgref;
	float x, y;
	int w, h;
	float s0, t0, s1, t1;
	float z; // used for drawing priority

	VisualEntity(IdPoolRef ref, IdPoolRef imgref, float x, float y, int w, int h, float s0, float t0, float s1, float t1, float z) : ref(ref), imgref(imgref), x(x), y(y), w(w), h(h), s0(s0), t0(t0), s1(s1), t1(t1), z(z) {}
};

class UICache final {
	std::vector<std::string> civs;
	Engine *e;
	std::vector<VisualEntity> entities;
	std::vector<IdPoolRef> selected;
	float left, top;
	ImDrawList *bkg;
public:
	void load(Engine &e);

	void idle();

	void user_interact_entities();

	/** Show user selected entities. */
	void show_selections();
	void show_entities();

	void show_mph_tbl(Frame&);
	void show_editor_menu();
	void show_editor_scenario();
private:
	void load_entities();

	void game_mouse_process();
};

}

}
