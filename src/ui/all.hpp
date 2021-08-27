#pragma once

#define IMGUI_INCLUDE_IMGUI_USER_H 1
#include "../imgui/imgui.h"

#include "../graphics.hpp"
#include "hud.hpp"

#include <memory>

namespace genie {

// TODO maak dit generieker voor gfx subsystem -> dat wordt texture & texturebuilder dus
class Preview final {
public:
	GLuint tex;
	SDL_Rect bnds; // x,y are hotspot x and y. w,h are size
	int offx, offy;
	std::string alt;

	Preview();
	Preview(const Preview&) = delete;
	~Preview();

	void load(const genie::Image &img, const SDL_Palette *pal);
	void show();
};

namespace ui {

class TexturesWidget final {
public:
	bool visible;

	void display();
};

// TODO move outside ui namespace
enum class UnitType {
	chariot_archer,
	archer,
	improved_archer,
	axeman,
	swordsman,
	ballista,
	stone_thrower,
	horse_archer,
	cataphract,
	max,
};

enum class UnitState {
	dead,
	die,
	idle,
	moving,
	attack,
	max,
};

class UnitInfo final {
public:
	unsigned attack_id, dead_id, decay_id, idle_id, moving_id;
	std::string name;

	UnitInfo(unsigned atk, unsigned die, unsigned decay, unsigned idle, unsigned moving, const std::string &name)
		: attack_id(atk), dead_id(die), decay_id(decay), idle_id(idle), moving_id(moving), name(name) {}

	UnitInfo(unsigned idle, const std::string &name)
		: attack_id(idle), dead_id(idle), decay_id(idle), idle_id(idle), moving_id(idle), name(name) {}
};

class UnitViewWidget final {
	int type;
	std::optional<genie::Animation> anim;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal;
	Preview preview;
	unsigned player;
	int offx, offy, subimage, state;
	bool changed, flip;
	const UnitInfo *info;
public:
	UnitViewWidget();

	void compute_offset();
	void set_anim(const UnitInfo &info, UnitState state);

	void show(bool &visible);
};

class DebugUI final {
	TexturesWidget tex;
	std::unique_ptr<UnitViewWidget> unitview;
	bool show_legacy, show_unitview;
public:
	DebugUI();

	void init_root() { unitview.reset(new UnitViewWidget()); }

	void display();
};

class UI final {
	bool m_show_debug;
	DebugUI dbg;
public:
	UI();

	constexpr bool show_debug() const noexcept { return m_show_debug; }
	void show_debug(bool enabled);

	void init_root() { dbg.init_root(); }

	void display();
};

}
}
