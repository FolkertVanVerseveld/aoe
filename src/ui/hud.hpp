/* Copyright 2019-2021 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#pragma once

#include "../imgui/imgui.h"

#include <map>
#include <set>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

namespace genie {

class Hud;

class UI {
public:
	bool hot, active, enabled, selected;
	SDL_Rect bnds;
	double left, top, right, bottom;

	unsigned id;

	UI() : UI(-1) {}
	UI(unsigned id) : hot(false), active(false), enabled(false), selected(false), bnds(), left(-1), top(-1), right(-1), bottom(-1), id(id) {}
	virtual ~UI() {}

	/** Display the component immediately. */
	virtual void display(Hud&) = 0;
	/** Apply new settings to component. */
	virtual bool update(Hud&) = 0;
};

class Popup {
public:
	bool opened, closed;
	std::vector<SDL_Rect> item_bounds;

	Popup() : opened(false), closed(false), item_bounds() {}

	virtual ~Popup() {}
	/** Display popup visuals. */
	virtual void display_popup(Hud&) = 0;
	virtual void item_select(unsigned index) = 0;
};

class Button : public UI {
public:
	SDL_Color cols[6];
	int shade, fnt_id;
	std::string label;

	Button(unsigned id) : UI(id), cols(), shade(), fnt_id(-1), label() { enabled = true; }

	void display(Hud&) override;
	bool update(Hud&) override;
};

class ListPopup : public UI, public Popup {
public:
	SDL_Color cols[6];
	int shade;
	unsigned fnt_id;
	std::string label;
	bool align_left;

	unsigned selected_item;
	const std::vector<std::string> &items;

	static constexpr int btn_w = 28, btn_h = 38;

	ListPopup(unsigned id, unsigned fnt_id, unsigned selected, const std::vector<std::string> &items)
		: UI(id), cols(), shade(), fnt_id(fnt_id), selected_item(selected), items(items), label(), align_left(false)
	{
		enabled = true;
	}

	void display(Hud&) override;
	void display_popup(Hud&) override;
	bool update(Hud&) override;

	void item_select(unsigned index) override {
		selected_item = index;
	}
};

/**
* Headup Display
*
* Note to self: never use GL_LINES to draw a bunch of lines next to each other,
* because this will always break when the is in fullscreen or custom resolution as gaps may appear.
* This can be mitigated by drawing rectangles on top of each other.
*/
class Hud final {
public:
	std::map<unsigned, std::unique_ptr<UI>> items;
	std::set<unsigned> keep;

	SDL_Color cols[6];
	double mouse_x, mouse_y;
	double left, right, top, bottom;
	bool align_left;
	int shade, fnt_id;
	Uint8 mouse_btn, mouse_btn_grab;
	std::string m_text;
	bool was_hot;
	genie::Texture *tex;

	//std::map<AnimationId, TileInfo> tiledata;
	Popup *ui_popup;

	void reset();
	void reset(genie::Texture &tex);

	void down(SDL_MouseButtonEvent &ev);
	void up(SDL_MouseButtonEvent &ev);

	bool try_grab(Uint8 mask, bool hot);
	bool lost_grab(Uint8 mask, bool active);

	void display();

	bool button(unsigned id, unsigned fnt_id, double x, double y, double w, double h, const std::string &label, int shade, SDL_Color cols[6]);
	bool listpopup(unsigned id, unsigned fnt_id, double x, double y, double w, double h, unsigned &selected, const std::vector<std::string> &elements, int shade, SDL_Color cols[6], bool left=true);

	void rect(SDL_Rect bnds, SDL_Color col[6]);
	void shaderect(SDL_Rect bnds, int shade);

	void text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const std::string &str, float width, int align);
	void text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const std::string &str, float width, int align, float &max_width);
	void text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align);
	void text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align, float &max_width);

	void textdim(ImFont *font, float size, ImVec2 &pos, const SDL_Rect &clip_rect, const std::string &str, float wrap_width, int align, float &max_width);
	void textdim(ImFont *font, float size, ImVec2 &pos, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align, float &max_width);

	void slow_text(int fnt_id, float size, ImVec2 pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align);
	void slow_text(int fnt_id, float size, ImVec2 pos, SDL_Color col, const SDL_Rect &clip_rect, const std::string &text, float wrap_width, int align);
	void title(ImVec2 pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align);
};

}
