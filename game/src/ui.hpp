#pragma once

#include "imgui_user.hpp"

#include "imfilebrowser.h"

#include <cstdint>

#include <initializer_list>

/**
 * Wrappers to make ImGui:: calls less tedious
 */

namespace aoe {

namespace ui {

class Frame final {
	bool open, active;
public:
	Frame();
	~Frame();

	bool begin(const char *name, ImGuiWindowFlags flags=0);
	bool begin(const char *name, bool &p_open, ImGuiWindowFlags flags=0);
	void close();

	void str(const char*);
	void fmt(const char*, ...);

	bool chkbox(const char*, bool&);
	bool btn(const char*, const ImVec2 &sz=ImVec2(0, 0));
	bool xbtn(const char*, const ImVec2 &sz=ImVec2(0, 0)); // like btn, but disabled
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

}

}
