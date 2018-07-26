/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdio>

#include <memory>
#include <string>
#include <stack>
#include <vector>

#include "../setup/def.h"
#include "../setup/res.h"

#include "gfx.h"
#include "lang.h"

extern struct pe_lib lib_lang;

/* load c-string from language dll and wrap into c++ string */
std::string load_string(unsigned id)
{
	char buf[4096];
	load_string(&lib_lang, id, buf, sizeof buf);
	return std::string(buf);
}

/* Custom renderer */
class Renderer {
public:
	SDL_Renderer *renderer;

	Renderer() {
		renderer = NULL;
	}

	void col(int r, int g, int b, int a = SDL_ALPHA_OPAQUE) {
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
	}

	void col(const SDL_Color &col) {
		SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
	}
} canvas;

bool point_in_rect(int x, int y, int w, int h, int px, int py)
{
	return px >= x && px < x + w && py >= y && py < y + h;
}

/**
 * Core User Interface element.
 * This is the minimum interface for anything User Interface related
 * (e.g. text, buttons)
 */
class UI {
protected:
	int x, y;
	unsigned w, h;

public:
	UI(int x, int y, unsigned w=1, unsigned h=1)
		: x(x), y(y), w(w), h(h) {}
	virtual ~UI() {}

	virtual void draw() const = 0;

	bool contains(int px, int py) {
		return point_in_rect(x, y, w, h, px, py);
	}
};

/** Text horizontal/vertical alignment */
enum TextAlign {
	LEFT = 0, TOP = 0,
	CENTER = 1, MIDDLE = 1,
	RIGHT = 2, BOTTOM = 2
};

const SDL_Color col_default = {255, 208, 157, SDL_ALPHA_OPAQUE};
const SDL_Color col_focus = {255, 255, 0, SDL_ALPHA_OPAQUE};

class Text final : public UI {
	std::string str;
	SDL_Surface *surf;
	SDL_Texture *tex;

public:
	Text(int x, int y, unsigned id
		, TextAlign halign=LEFT
		, TextAlign valign=TOP
		, TTF_Font *fnt=fnt_default
		, SDL_Color col=col_default)
		: UI(x, y), str(load_string(id))
	{
		surf = TTF_RenderText_Solid(fnt, str.c_str(), col);
		tex = SDL_CreateTextureFromSurface(renderer, surf);

		this->w = surf->w;
		this->h = surf->h;

		switch (halign) {
		case LEFT: break;
		case CENTER: this->x -= (int)w / 2; break;
		case RIGHT: this->x -= (int)w; break;
		}

		switch (valign) {
		case TOP: break;
		case MIDDLE: this->y -= (int)h / 2; break;
		case BOTTOM: this->y -= (int)h; break;
		}
	}

	~Text() {
		SDL_DestroyTexture(tex);
		SDL_FreeSurface(surf);
	}

public:
	void draw() const {
		SDL_Rect pos = {x, y, (int)w, (int)h};
		SDL_RenderCopy(renderer, tex, NULL, &pos);
	}
};

// FIXME more color schemes
class Border : public UI {
public:
	Border(int x, int y, unsigned w=1, unsigned h=1)
		: UI(x, y, w, h) {}

	void draw() const {
		draw(false);
	}

	void draw(bool invert) const {
		unsigned w = this->w - 1, h = this->h - 1;

		const SDL_Color cols[] = {
			{41 , 33 , 16, SDL_ALPHA_OPAQUE},
			{145, 136, 71, SDL_ALPHA_OPAQUE},
			{78 , 61 , 49, SDL_ALPHA_OPAQUE},
			{129, 112, 65, SDL_ALPHA_OPAQUE},
			{107, 85 , 34, SDL_ALPHA_OPAQUE},
			{97 , 78 , 50, SDL_ALPHA_OPAQUE},
		};

		int table[] = {0, 1, 2, 3, 4, 5}, table_r[] = {1, 0, 3, 2, 5, 4};
		int *colptr = invert ? table_r : table;

		// Draw outermost lines
		canvas.col(cols[colptr[0]]);
		SDL_RenderDrawLine(renderer, x, y    , x    , y + h);
		SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
		canvas.col(cols[colptr[1]]);
		SDL_RenderDrawLine(renderer, x + 1, y, x + w, y        );
		SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h - 1);
		// Draw middle lines
		canvas.col(cols[colptr[2]]);
		SDL_RenderDrawLine(renderer, x + 1, y + 1    , x + 1    , y + h - 1);
		SDL_RenderDrawLine(renderer, x + 1, y + h - 1, x + w - 1, y + h - 1);
		canvas.col(cols[colptr[3]]);
		SDL_RenderDrawLine(renderer, x + 2    , y + 1, x + w - 1, y + 1    );
		SDL_RenderDrawLine(renderer, x + w - 1, y + 1, x + w - 1, y + h - 2);
		// Draw innermost lines
		canvas.col(cols[colptr[4]]);
		SDL_RenderDrawLine(renderer, x + 2, y + 2    , x + 2    , y + h - 2);
		SDL_RenderDrawLine(renderer, x + 2, y + h - 2, x + w - 2, y + h - 2);
		canvas.col(cols[colptr[5]]);
		SDL_RenderDrawLine(renderer, x + 3    , y + 2, x + w - 2, y + 2    );
		SDL_RenderDrawLine(renderer, x + w - 2, y + 2, x + w - 2, y + h - 3);
	}
};

class Button final : public Border {
	Text text, text_focus;
public:
	bool focus;
	bool down;

	Button(int x, int y, unsigned w, unsigned h, unsigned id, bool def_fnt=false)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, id, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button)
		, text_focus(x + w / 2, y + h / 2, id, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button, col_focus)
		, focus(false), down(false)
	{
	}

	// NOTE return value of mousedown indicates whether it is dirty
	// but return value of mouseup indicates whether it has been clicked!!

	bool mousedown(SDL_MouseButtonEvent *event) {
		if (down = contains(event->x, event->y))
			focus = true;
		return down;
	}

	bool mouseup(SDL_MouseButtonEvent *event) {
		down = false;
		return contains(event->x, event->y);
	}

	void draw() const {
		Border::draw(down);
		if (focus)
			text_focus.draw();
		else
			text.draw();
	}
};

/* Provides a group of buttons the user also can navigate through with arrow keys */
class ButtonGroup final : public UI {
	std::vector<std::shared_ptr<Button>> objects;
	unsigned old_focus = 0;
	bool down_ = false;

public:
	unsigned focus = 0;

	ButtonGroup(int x=212, int y=222, unsigned w=375, unsigned h=50)
		: UI(x, y, w, h), objects() {}

	void add(int rel_x, int rel_y, unsigned id, unsigned w=0, unsigned h=0) {
		if (!w) w = this->w;
		if (!h) h = this->h;

		objects.emplace_back(new Button(x + rel_x, y + rel_y, w, h, id));
	}

	void update() {
		auto old = objects[old_focus].get();
		auto next = objects[focus].get();

		old->focus = false;
		next->focus = true;
	}

	bool ror() {
		if (down_)
			return false;
		old_focus = focus;
		focus = (focus + 1) % objects.size();
		return true;
	}

	bool rol() {
		if (down_)
			return false;
		old_focus = focus;
		focus = (focus + objects.size() - 1) % objects.size();
		return true;
	}

	bool mousedown(SDL_MouseButtonEvent *event) {
		bool dirty = false;
		unsigned id = 0;

		old_focus = focus;

		for (auto x : objects) {
			auto btn = x.get();

			if (btn->mousedown(event)) {
				dirty = true;
				focus = id;
			} else
				btn->focus = false;

			++id;
		}

		return dirty;
	}

	bool mouseup(SDL_MouseButtonEvent *event) {
		return objects[focus].get()->mouseup(event);
	}

	void down() {
		down_ = true;
		objects[focus].get()->down = true;
	}

	void up() {
		down_ = false;
		objects[focus].get()->down = false;
	}

	void draw() const {
		for (auto x : objects)
			x.get()->draw();
	}
};

class Menu : public UI {
protected:
	std::vector<std::shared_ptr<UI>> objects;
	mutable ButtonGroup group;
public:
	bool stop = false;
	bool terminate = false;

	Menu() : UI(0, 0, WIDTH, HEIGHT), objects(), group() {}

	virtual void draw() const {
		for (auto x : objects)
			x.get()->draw();

		group.update();
		group.draw();
	}

	bool keydown(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;
		bool dirty = false;

		if (virt == SDLK_DOWN)
			return group.ror();
		else if (virt == SDLK_UP)
			return group.rol();
		else if (virt == ' ') {
			group.down();
			dirty = true;
		}

		return dirty;
	}

	bool keyup(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;
		bool dirty = false;

		if (virt == ' ') {
			dirty = true;
			group.up();
			button_group_activate(group.focus);
		}
		return dirty;
	}

	bool mousedown(SDL_MouseButtonEvent *event) {
		if (group.mousedown(event))
			return true;

		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
			// FIXME focus propagation (for all other buttons: focus=false)
			if (btn && btn->mousedown(event))
				return true;
		}

		return false;
	}

	bool mouseup(SDL_MouseButtonEvent *event) {
		if (group.mouseup(event)) {
			button_group_activate(group.focus);
			return true;
		}

		unsigned id = 0;
		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
			if (btn && btn->mouseup(event)) {
				button_activate(id);
				return true;
			}
			++id;
		}

		return false;
	}

	virtual bool button_group_activate(unsigned id) = 0;
	virtual bool button_activate(unsigned id) = 0;
};

std::stack<std::shared_ptr<Menu>> ui_navigation;

// FIXME color scheme
class MenuSinglePlayer final : public Menu {
public:
	MenuSinglePlayer() : Menu() {
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(0, 147 - 222, STR_BTN_RANDOM_MAP);
		group.add(0, 210 - 222, STR_BTN_CAMPAIGN);
		group.add(0, 272 - 222, STR_BTN_DEATHMATCH);
		group.add(0, 335 - 222, STR_BTN_SCENARIO);
		group.add(0, 397 - 222, STR_BTN_SAVEDGAME);
		group.add(0, 460 - 222, STR_BTN_CANCEL);
	}

	bool button_group_activate(unsigned id) override final {
		switch (id) {
		case 5:
			return stop = true;
		}
		return false;
	}

	bool button_activate(unsigned) override final {
		return terminate = stop = true;
	}
};

class MenuMain final : public Menu {
public:
	MenuMain() : Menu() {
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));

		group.add(0, 0, STR_BTN_SINGLEPLAYER);
		group.add(0, 285 - 222, STR_BTN_MULTIPLAYER);
		group.add(0, 347 - 222, STR_BTN_HELP);
		group.add(0, 410 - 222, STR_BTN_EDIT);
		group.add(0, 472 - 222, STR_BTN_EXIT);

		// FIXME (tm) gets truncated by resource handling in res.h (ascii, unicode stuff)
		objects.emplace_back(new Text(WIDTH / 2, 542, STR_MAIN_COPY1, CENTER));
		// FIXME (copy) and (p) before this line
		objects.emplace_back(new Text(WIDTH / 2, 561, STR_MAIN_COPY2, CENTER));
		objects.emplace_back(new Text(WIDTH / 2, 578, STR_MAIN_COPY3, CENTER));
	}

	bool button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_navigation.emplace(new MenuSinglePlayer());
			return true;
		case 4:
			return stop = true;
		}
		return false;
	}

	bool button_activate(unsigned) override final {
		panic("Unimplemented");
		return true;
	}
};

bool button_epilogue(bool dirty, Menu *menu)
{
	if (menu->terminate) {
		while (!ui_navigation.empty())
			ui_navigation.pop();
		dirty = true;
	} else if (menu->stop)
		ui_navigation.pop();

	return dirty;
}

extern "C"
{

void ui_init()
{
	canvas.renderer = renderer;
	ui_navigation.emplace(new MenuMain());
}

void ui_free(void)
{
}

bool display()
{
	if (ui_navigation.empty())
		return false;

	ui_navigation.top().get()->draw();
	return true;
}

bool keydown(SDL_KeyboardEvent *event)
{
	if (ui_navigation.empty())
		return true;
	return ui_navigation.top().get()->keydown(event);
}

bool keyup(SDL_KeyboardEvent *event)
{
	if (ui_navigation.empty())
		return true;

	auto menu = ui_navigation.top().get();
	bool dirty = menu->keyup(event);

	return button_epilogue(dirty, menu);
}

bool mousedown(SDL_MouseButtonEvent *event)
{
	if (ui_navigation.empty())
		return true;

	return ui_navigation.top().get()->mousedown(event);
}

bool mouseup(SDL_MouseButtonEvent *event)
{
	if (ui_navigation.empty())
		return true;

	auto menu = ui_navigation.top().get();
	bool dirty = menu->mouseup(event);

	return button_epilogue(dirty, menu);
}

}
