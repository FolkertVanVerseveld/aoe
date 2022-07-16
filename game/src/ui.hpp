#pragma once

#include "imgui_user.hpp"

#include <cstdint>

#include <initializer_list>

/**
 * Wrappers to make ImGui:: calls less tedious
 */

namespace aoe {

namespace ui {

class Frame final {
	bool open;
public:
	Frame();
	~Frame();

	bool begin(const char *name, ImGuiWindowFlags flags=0);
	bool begin(const char *name, bool &p_open, ImGuiWindowFlags flags=0);
	void close();

	void str(const char*);
	bool chkbox(const char*, bool&);
	bool btn(const char*, const ImVec2 &sz=ImVec2(0, 0));

	bool scalar(const char *label, uint32_t &v);

	void sl(); // SameLine

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

	void str(const char*);
	void strs(std::initializer_list<const char*>);

	void fmt(const char *s, ...);
};

}

}
