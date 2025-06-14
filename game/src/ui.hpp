#pragma once

#include "ui/imgui_user.hpp"

#include "external/imfilebrowser.h"

#include <idpool.hpp>

#include "legacy/strings.hpp"
#include "legacy/scenario.hpp"
#include "engine/assets.hpp"

#include "external/imgui_memory_editor.h"

#include "game.hpp"

#include <SDL2/SDL_rect.h>

#include <cstdint>

#include <initializer_list>
#include <vector>

/**
 * Wrappers to make ImGui:: calls less tedious
 */

namespace aoe {

class Assets;
class Engine;

void DrawLine(ImDrawList *lst, float x0, float y0, float x1, float y1, SDL_Color col);

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
	void txt2(StrId, TextHalign ha=TextHalign::left, const ImVec4 &bg=ImVec4(0, 0, 0, 1));

	void str(const std::string&);
	void str(const char*);
	void str(StrId);
	void fmt(const char*, ...);

	bool chkbox(const char*, bool&);
	bool btn(const char*, TextHalign ha=TextHalign::left, const ImVec2 &sz=ImVec2(0, 0));
	bool xbtn(const char*, TextHalign ha=TextHalign::left, const ImVec2 &sz=ImVec2(0, 0)); // like btn, but disabled
	bool xbtn(const char*, const char *tooltip, TextHalign ha=TextHalign::left, const ImVec2 &sz=ImVec2(0, 0)); // like btn, but disabled
	bool combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height_in_items=-1);

	// typed input fields
	bool scalar(const char *label, int32_t &v, int32_t step=0);
	bool scalar(const char *label, uint32_t &v, uint32_t step=0);
	// like scalar but clamps the passed v
	bool scalar(const char *label, int32_t &v, int32_t step, int32_t min, int32_t max);
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
	bool xflip;

	VisualEntity(IdPoolRef ref, IdPoolRef imgref, float x, float y, int w, int h, float s0, float t0, float s1, float t1, float z, bool xflip) : ref(ref), imgref(imgref), x(x), y(y), w(w), h(h), s0(s0), t0(t0), s1(s1), t1(t1), z(z), xflip(xflip) {}
};

#undef small

struct VisualTile final {
	int tx, ty;
	SDL_Rect bnds;

	VisualTile(int tx, int ty, const SDL_Rect &bnds) : tx(tx), ty(ty), bnds(bnds) {}
};

void show_player_game_table(ui::Frame &f);
void show_scenario_settings(ui::Frame &f, ScenarioSettings &scn);

enum class HudState {
	start,
	worker,
	buildmenu,
	trainmenu,
};

class UICache final {
	std::vector<std::string> civs;
	Engine *e;
	std::vector<VisualEntity> entities, entities_deceased;
	std::vector<VisualEntity> particles; // recycle for 'visual particle'
	std::vector<IdPoolRef> selected;
	std::vector<VisualTile> display_area;
	float left, top, scale;
	ImDrawList *bkg;
	std::string btnsel;

	std::vector<ImageSet> t_imgs;
	ImGui::FileBrowser fd; // for scn or cpx
	ImGui::FileBrowser fd2; // for saving scn or cpx
	io::Scenario scn;
	ScenarioEditor scn_edit;
	Game scn_game;
	MemoryEditor mem;
	SDL_Rect gmb_top, gmb_bottom; // game menu bars
	bool select_started, multi_select, btn_left;
	int build_menu;
	float start_x, start_y;
public:
	HudState state;

	UICache();

	std::optional<Entity> first_selected_entity();
	std::optional<Entity> first_selected_building();
	std::optional<Entity> first_selected_worker();

	bool try_select(EntityType type, unsigned playerid);
	bool find_idle_villager(unsigned playerid);

	void load();

	void idle(Engine &e);
	void idle_game();
	void idle_editor(Engine &e);
	void show_multiplayer_game();

	void try_kill_first_entity();

	void show_world();

	void show_mph_tbl(Frame&);
	void show_editor_menu();
	void show_editor_scenario();

	void show_hud_selection(float left, float top, float menubar_h);

	void str2(const ImVec2 &pos, const char *text, bool invert=false);
	void strnum(const ImVec2 &pos, int v);

	void str_scream(const char *txt);

	const gfx::ImageRef &imgtile(uint8_t v);

	void set_scn(io::Scenario&);

	void try_open_build_menu();
private:
	void play_sfx(SfxId id, int loops=0);
	void draw_tile(uint8_t id, uint8_t h, int x, int y, const ImVec2 &size, ImU32 col);
	void show_terrain();
	/** Show user selected entities. */
	void show_selections();
	void show_entities();
	void show_particles();

	void load_entities();

	void game_mouse_process();
	void mouse_left_process();
	void mouse_right_process();

	bool menu_btn(ImTextureID tex, const Assets &a, const char *lbl, float x, float scale, bool small);
	bool frame_btn(const BackgroundColors &col, const char *lbl, float x, float y, float w, float h, float scale, bool invert=false);

	void collect(std::vector<IdPoolRef> &refs, const SDL_Rect &area, bool filter=true);
	void collect(std::vector<IdPoolRef> &refs, float mx, float my, bool filter=true);

	void image(const gfx::ImageRef &ref, float x, float y, float scale);

	unsigned player_index() const;
};

}

}
