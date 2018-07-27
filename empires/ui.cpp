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
#include "game.hpp"

extern struct pe_lib lib_lang;

/* load c-string from language dll and wrap into c++ string */
std::string load_string(unsigned id)
{
	char buf[4096];
	load_string(&lib_lang, id, buf, sizeof buf);
	return std::string(buf);
}

class Menu;

/**
 * Global User Interface state handler.
 * This `state machine' determines when the screen has to be redisplayed
 * and does some navigation logic as well.
 */
class UI_State {
	unsigned state;

	static unsigned const DIRTY = 1;
	static unsigned const BUTTON_DOWN = 2;

	std::stack<std::shared_ptr<Menu>> navigation;
public:
	UI_State() : state(DIRTY), navigation() {}

	void mousedown(SDL_MouseButtonEvent *event);
	void mouseup(SDL_MouseButtonEvent *event);
	void keydown(SDL_KeyboardEvent *event);
	void keyup(SDL_KeyboardEvent *event);

	/* Push new menu onto navigation stack */
	void go_to(Menu *menu);

	void dirty() {
		state |= DIRTY;
	}

	void display();
	void dispose();
} ui_state;

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

const SDL_Color col_players[MAX_PLAYER_COUNT] = {
	{0, 0, 196, SDL_ALPHA_OPAQUE},
	{200, 0, 0, SDL_ALPHA_OPAQUE},
	{234, 234, 0, SDL_ALPHA_OPAQUE},
	{140, 90, 33, SDL_ALPHA_OPAQUE},
	{255, 128, 0, SDL_ALPHA_OPAQUE},
	{0, 128, 0, SDL_ALPHA_OPAQUE},
	{128, 128, 128, SDL_ALPHA_OPAQUE},
	{64, 128, 128, SDL_ALPHA_OPAQUE},
};

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

		reshape(halign, valign);
	}

	Text(int x, int y, const std::string &str
		, TextAlign halign=LEFT
		, TextAlign valign=TOP
		, TTF_Font *fnt=fnt_default
		, SDL_Color col=col_default)
		: UI(x, y), str(str)
	{
		surf = TTF_RenderText_Solid(fnt, str.c_str(), col);
		tex = SDL_CreateTextureFromSurface(renderer, surf);

		reshape(halign, valign);
	}

	~Text() {
		SDL_DestroyTexture(tex);
		SDL_FreeSurface(surf);
	}

private:
	void reshape(TextAlign halign, TextAlign valign) {
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

	Button(int x, int y, unsigned w, unsigned h, const std::string &str, bool def_fnt=false)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, str, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button)
		, text_focus(x + w / 2, y + h / 2, str, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button, col_focus)
		, focus(false), down(false)
	{
	}

	/* Process mouse event if it has been clicked. */
	bool mousedown(SDL_MouseButtonEvent *event) {
		bool old_down = down;

		focus = down = contains(event->x, event->y);

		if (old_down != down)
			ui_state.dirty();

		return down;
	}

	/* Process mouse event if it has been clicked. */
	bool mouseup(SDL_MouseButtonEvent *event) {
		if (down) {
			down = false;
			ui_state.dirty();

			return contains(event->x, event->y);
		}

		// Button wasn't pressed so we can't munge the event.
		return false;
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

	void add(int rel_x, int rel_y, unsigned id=STR_ERROR, unsigned w=0, unsigned h=0, bool def_fnt=false) {
		if (!w) w = this->w;
		if (!h) h = this->h;

		objects.emplace_back(new Button(x + rel_x, y + rel_y, w, h, id, def_fnt));
	}

	void update() {
		auto old = objects[old_focus].get();
		auto next = objects[focus].get();

		old->focus = false;
		next->focus = true;
	}

	/* Rotate right through button group */
	void ror() {
		if (down_)
			return;

		old_focus = focus;
		focus = (focus + 1) % objects.size();

		if (old_focus != focus)
			ui_state.dirty();
	}

	/* Rotate left through button group */
	void rol() {
		if (down_)
			return;

		old_focus = focus;
		focus = (focus + objects.size() - 1) % objects.size();

		if (old_focus != focus)
			ui_state.dirty();
	}

	void mousedown(SDL_MouseButtonEvent *event) {
		unsigned id = 0;
		old_focus = focus;

		for (auto x : objects) {
			auto btn = x.get();

			if (btn->mousedown(event))
				focus = id;
			else
				btn->focus = false;

			++id;
		}
	}

	bool mouseup(SDL_MouseButtonEvent *event) {
		return objects[focus].get()->mouseup(event);
	}

	void down() {
		down_ = true;
		objects[focus].get()->down = true;
		ui_state.dirty();
	}

	void up() {
		down_ = false;
		objects[focus].get()->down = false;
		ui_state.dirty();
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
	int stop = 0;

	Menu(unsigned title_id, bool show_title=true)
		: UI(0, 0, WIDTH, HEIGHT), objects(), group()
	{
		if (show_title)
			objects.emplace_back(new Text(
				WIDTH / 2, 12, title_id, MIDDLE, TOP, fnt_button
			));
	}

	Menu(unsigned title_id, int x, int y, unsigned w, unsigned h, bool show_title=true)
		: UI(0, 0, WIDTH, HEIGHT), objects(), group(x, y, w, h)
	{
		if (show_title)
			objects.emplace_back(new Text(
				WIDTH / 2, 12, title_id, MIDDLE, TOP, fnt_button
			));
	}

	virtual void draw() const {
		for (auto x : objects)
			x.get()->draw();

		group.update();
		group.draw();
	}

	void keydown(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;

		if (virt == SDLK_DOWN)
			group.ror();
		else if (virt == SDLK_UP)
			group.rol();
		else if (virt == ' ')
			group.down();
	}

	void keyup(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;

		if (virt == ' ') {
			group.up();
			button_group_activate(group.focus);
		}
	}

	void mousedown(SDL_MouseButtonEvent *event) {
		group.mousedown(event);

		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
			if (btn)
				btn->mousedown(event);
		}
	}

	void mouseup(SDL_MouseButtonEvent *event) {
		if (group.mouseup(event)) {
			button_group_activate(group.focus);
			return;
		}

		unsigned id = 0;
		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
			if (btn && btn->mouseup(event))
				button_activate(id);
			++id;
		}
	}

	virtual void button_group_activate(unsigned id) = 0;

	virtual void button_activate(unsigned) {
		panic("Unimplemented");
	}
};

class MenuTimeline final : public Menu {
public:
	MenuTimeline() : Menu(STR_TITLE_ACHIEVEMENTS, 0, 0, 550 - 250, 588 - 551) {
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		objects.emplace_back(new Text(WIDTH / 2, 48, STR_BTN_TIMELINE, CENTER, TOP));

		// TODO compute elapsed time
		objects.emplace_back(new Text(685, 15, "00:00:00"));

		objects.emplace_back(new Border(12, 106, 787 - 12, 518 - 106));

		group.add(250, 551, STR_BTN_BACK);

		unsigned i = 1, step = (474 - 151) / (players.size() + 1);

		for (auto x : players) {
			auto p = x.get();
			char buf[64];
			char civbuf[64];
			load_string(&lib_lang, STR_CIV_EGYPTIAN + p->civ, civbuf, sizeof civbuf);
			snprintf(buf, sizeof buf, "%s - %s", p->name.c_str(), civbuf);

			objects.emplace_back(new Text(40, 151 + i * step , buf, LEFT, MIDDLE));
			++i;
		}
	}

	virtual void button_group_activate(unsigned) override final {
		stop = 1;
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 2: running = 0; break;
		}
	}

	virtual void draw() const override {
		canvas.col(255, 255, 255);
		SDL_RenderDrawLine(renderer, 37, 151, 765, 151);
		SDL_RenderDrawLine(renderer, 765, 151, 765, 474);
		SDL_RenderDrawLine(renderer, 37, 151, 37, 474);
		SDL_RenderDrawLine(renderer, 37, 474, 764, 474);
		Menu::draw();
	}
};

class MenuAchievements final : public Menu {
	bool end;
public:
	MenuAchievements(bool end=false) : Menu(STR_TITLE_ACHIEVEMENTS, 0, 0, end ? 775 - 550 : 375 - 125, 588 - 551), end(end) {
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		objects.emplace_back(new Text(WIDTH / 2, 48, STR_TITLE_SUMMARY, CENTER, TOP));

		// TODO compute elapsed time
		objects.emplace_back(new Text(685, 15, "00:00:00"));

		if (end) {
			group.add(550, 551, STR_BTN_CLOSE);
			group.add(25, 551, STR_BTN_TIMELINE);
			group.add(287, 551, STR_BTN_RESTART);
		} else {
			group.add(425, 551, STR_BTN_CLOSE);
			group.add(125, 551, STR_BTN_TIMELINE);
		}

		objects.emplace_back(new Button(124, 187, 289 - 124, 212 - 187, STR_BTN_MILITARY, LEFT));
		objects.emplace_back(new Button(191, 162, 354 - 191, 187 - 162, STR_BTN_ECONOMY, LEFT));
		objects.emplace_back(new Button(266, 137, 420 - 266, 162 - 137, STR_BTN_RELIGION, LEFT));
		objects.emplace_back(new Button(351, 112, 549 - 351, 137 - 112, STR_BTN_TECHNOLOGY));
		//objects.emplace_back(new Text(485, 137, 639 - 485, 162 - 137, STR_SURVIVAL, RIGHT, TOP, fnt_default));
		//objects.emplace_back(new Text(552, 162, 713 - 552, 187 - 162, STR_WONDER, RIGHT, TOP, fnt_default));
		//objects.emplace_back(new Text(617, 187, 781 - 617, 212 - 187, STR_TOTAL_SCORE, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(633, 140, STR_SURVIVAL, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(710, 165, STR_WONDER, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(778, 190, STR_TOTAL_SCORE, RIGHT, TOP, fnt_default));

		unsigned y = 225, nr = 0;

		for (auto x : players) {
			auto p = x.get();

			objects.emplace_back(new Text(31, y, p->name, LEFT, TOP, fnt_default, col_players[nr % MAX_PLAYER_COUNT]));

			y += 263 - 225;
			++nr;
		}
	}

	virtual void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0: stop = end ? 5 : 1; break;
		case 1: ui_state.go_to(new MenuTimeline()); break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 2: running = 0; break;
		}
	}
};

// TODO make overlayed menu
class MenuGameMenu final : public Menu {
public:
	MenuGameMenu() : Menu(STR_TITLE_MAIN, 200, 98, 580 - 220, 143 - 113, false) {
		group.add(220 - 200, 113 - 98, STR_BTN_GAME_QUIT);
		group.add(220 - 200, 163 - 98, STR_BTN_GAME_ACHIEVEMENTS);
		group.add(220 - 200, 198 - 98, STR_BTN_GAME_INSTRUCTIONS);
		group.add(220 - 200, 233 - 98, STR_BTN_GAME_SAVE);
		group.add(220 - 200, 268 - 98, STR_BTN_GAME_LOAD);
		group.add(220 - 200, 303 - 98, STR_BTN_GAME_RESTART);
		group.add(220 - 200, 338 - 98, STR_BTN_GAME_SETTINGS);
		group.add(220 - 200, 373 - 98, STR_BTN_HELP);
		group.add(220 - 200, 408 - 98, STR_BTN_GAME_ABOUT);
		group.add(220 - 200, 458 - 98, STR_BTN_GAME_CANCEL);

		objects.emplace_back(new Border(200, 98, 600 - 200, 503 - 98));
	}

	virtual void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0: ui_state.go_to(new MenuAchievements(true)); break;
		case 1: ui_state.go_to(new MenuAchievements()); break;
		case 9: stop = 1; break;
		}
	}
};

class MenuGame final : public Menu {
public:
	MenuGame() : Menu(STR_TITLE_MAIN, 0, 0, 728 - 620, 18, false) {
		group.add(728, 0, STR_BTN_MENU, WIDTH - 728, 18, true);
		group.add(620, 0, STR_BTN_DIPLOMACY, 728 - 620, 18, true);

		objects.emplace_back(new Text(WIDTH / 2, 3, STR_AGE_STONE, MIDDLE, TOP));
		objects.emplace_back(new Text(WIDTH / 2, HEIGHT / 2, STR_PAUSED, MIDDLE, CENTER, fnt_button));

		const Player *you = players[0].get();

		char buf[32];
		int x = 4;
		snprintf(buf, sizeof buf, "F: %u", you->resources.food);
		objects.emplace_back(new Text(x, 3, buf));
		x += 80;
		snprintf(buf, sizeof buf, "W: %u", you->resources.wood);
		objects.emplace_back(new Text(x, 3, buf));
		x += 80;
		snprintf(buf, sizeof buf, "G: %u", you->resources.gold);
		objects.emplace_back(new Text(x, 3, buf));
		x += 80;
		snprintf(buf, sizeof buf, "S: %u", you->resources.stone);
		objects.emplace_back(new Text(x, 3, buf));

		objects.emplace_back(new Button(765, 482, 795 - 765, 512 - 482, STR_BTN_SCORE, true));
		objects.emplace_back(new Button(765, 564, 795 - 765, 594 - 564, "?", true));
		//objects.emplace_back(new Button(765, 564, "?"));
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuGameMenu());
			break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 0: break;
		case 1: break;
		}
	}
};

class MenuSinglePlayerSettings final : public Menu {
public:
	MenuSinglePlayerSettings() : Menu(STR_TITLE_SINGLEPLAYER, 0, 0, 386 - 87, 586 - 550) {
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(87, 550, STR_BTN_START_GAME);
		group.add(412, 550, STR_BTN_CANCEL);
		group.add(525, 62, STR_BTN_SETTINGS, 786 - 525, 98 - 62);

		// TODO add missing UI objects

		// setup players
		players.clear();
		players.emplace_back(new PlayerHuman("You"));
		players.emplace_back(new PlayerComputer());
		players.emplace_back(new PlayerComputer());
		players.emplace_back(new PlayerComputer());

		char str_count[2] = "0";
		str_count[0] += players.size();

		// FIXME hardcoded 2 player mode
		objects.emplace_back(new Text(38, 374, STR_PLAYER_COUNT, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(44, 410, str_count));
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuGame());
			break;
		case 1:
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 2: running = 0; break;
		}
	}
};

// FIXME color scheme
class MenuSinglePlayer final : public Menu {
public:
	MenuSinglePlayer() : Menu(STR_TITLE_SINGLEPLAYER_MENU) {
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(0, 147 - 222, STR_BTN_RANDOM_MAP);
		group.add(0, 210 - 222, STR_BTN_CAMPAIGN);
		group.add(0, 272 - 222, STR_BTN_DEATHMATCH);
		group.add(0, 335 - 222, STR_BTN_SCENARIO);
		group.add(0, 397 - 222, STR_BTN_SAVEDGAME);
		group.add(0, 460 - 222, STR_BTN_CANCEL);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuSinglePlayerSettings());
			break;
		case 5:
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned) override final {
		running = 0;
	}
};

class MenuMain final : public Menu {
public:
	MenuMain() : Menu(STR_TITLE_MAIN) {
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

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuSinglePlayer());
			break;
		case 4:
			running = 0;
			break;
		}
	}
};

extern "C"
{

void ui_init()
{
	canvas.renderer = renderer;
	ui_state.go_to(new MenuMain());
}

void ui_free(void)
{
	running = 0;
	ui_state.dispose();
}

/* Wrappers for ui_state */
void display() { ui_state.display(); }

void keydown(SDL_KeyboardEvent *event) { ui_state.keydown(event); }
void keyup(SDL_KeyboardEvent *event) { ui_state.keyup(event); }

void mousedown(SDL_MouseButtonEvent *event) { ui_state.mousedown(event); }
void mouseup(SDL_MouseButtonEvent *event) { ui_state.mouseup(event); }

}

void UI_State::mousedown(SDL_MouseButtonEvent *event)
{
	if (navigation.empty())
		return;

	navigation.top().get()->mousedown(event);
}

void UI_State::mouseup(SDL_MouseButtonEvent *event)
{
	if (navigation.empty())
		return;

	auto menu = navigation.top().get();
	menu->mouseup(event);

	for (unsigned n = menu->stop; n; --n) {
		if (navigation.empty())
			return;
		navigation.pop();
	}
}

void UI_State::go_to(Menu *menu)
{
	navigation.emplace(menu);
}

void UI_State::dispose()
{
	//puts("ui_state: dispose");
	while (!navigation.empty())
		navigation.pop();
}

void UI_State::display()
{
	//puts("ui_state: display");

	if ((state & DIRTY) && !navigation.empty()) {
		//puts("ui_state: redraw");
		// Clear screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
		// Draw stuff
		navigation.top().get()->draw();
		// Swap buffers
		SDL_RenderPresent(renderer);
	}

	if (!running)
		dispose();

	state &= ~DIRTY;
}

void UI_State::keydown(SDL_KeyboardEvent *event)
{
	if (navigation.empty())
		return;

	navigation.top().get()->keydown(event);
}

void UI_State::keyup(SDL_KeyboardEvent *event)
{
	if (navigation.empty())
		return;

	auto menu = navigation.top().get();
	menu->keyup(event);

	for (unsigned n = menu->stop; n; --n) {
		if (navigation.empty())
			return;
		navigation.pop();
	}
}
