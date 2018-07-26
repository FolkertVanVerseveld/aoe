/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdio>

#include <string>
#include <memory>
#include <vector>

#include "../setup/res.h"

#include "gfx.h"
#include "lang.h"

extern struct pe_lib lib_lang;

std::string load_string(unsigned id)
{
	char buf[4096];
	load_string(&lib_lang, id, buf, sizeof buf);
	return std::string(buf);
}

class UI {
protected:
	int x, y;
	unsigned w, h;

public:
	UI(int x, int y, unsigned w=1, unsigned h=1)
		: x(x), y(y), w(w), h(h) {}
	virtual ~UI() {}

	virtual void draw(SDL_Renderer *renderer) const = 0;
};

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
	Text(SDL_Renderer *renderer
		, int x, int y, unsigned id
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
	void draw(SDL_Renderer *renderer) const {
		SDL_Rect pos = {x, y, (int)w, (int)h};
		SDL_RenderCopy(renderer, tex, NULL, &pos);
	}
};

class Border : public UI {
public:
	Border(int x, int y, unsigned w=1, unsigned h=1)
		: UI(x, y, w, h) {}

	void draw(SDL_Renderer *renderer) const {
		unsigned w = this->w - 1, h = this->h - 1;
		// Draw outermost lines
		SDL_SetRenderDrawColor(renderer, 41, 33, 16, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x, y    , x    , y + h);
		SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
		SDL_SetRenderDrawColor(renderer, 145, 136, 71, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x + 1, y, x + w, y        );
		SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h - 1);
		// Draw middle lines
		SDL_SetRenderDrawColor(renderer, 78, 61, 49, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x + 1, y + 1    , x + 1    , y + h - 1);
		SDL_RenderDrawLine(renderer, x + 1, y + h - 1, x + w - 1, y + h - 1);
		SDL_SetRenderDrawColor(renderer, 129, 112, 65, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x + 2    , y + 1, x + w - 1, y + 1    );
		SDL_RenderDrawLine(renderer, x + w - 1, y + 1, x + w - 1, y + h - 2);
		// Draw innermost lines
		SDL_SetRenderDrawColor(renderer, 107, 85, 34, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x + 2, y + 2    , x + 2    , y + h - 2);
		SDL_RenderDrawLine(renderer, x + 2, y + h - 2, x + w - 2, y + h - 2);
		SDL_SetRenderDrawColor(renderer, 97, 78, 50, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(renderer, x + 3    , y + 2, x + w - 2, y + 2    );
		SDL_RenderDrawLine(renderer, x + w - 2, y + 2, x + w - 2, y + h - 3);
	}
};

class Button final : public Border {
	Text text, text_focus;
	bool focus;
public:
	Button(SDL_Renderer *renderer, int x, int y, unsigned w, unsigned h, unsigned id, bool focus=false)
		: Border(x, y, w, h)
		, text(renderer, x + w / 2, y + h / 2, id, CENTER, MIDDLE, fnt_button)
		, text_focus(renderer, x + w / 2, y + h / 2, id, CENTER, MIDDLE, fnt_button, col_focus)
		, focus(focus)
	{
	}

	void draw(SDL_Renderer *renderer) const {
		Border::draw(renderer);
		if (focus)
			text_focus.draw(renderer);
		else
			text.draw(renderer);
	}
};

std::vector<std::shared_ptr<UI>> ui_objects;

extern "C"
{

void ui_init(SDL_Renderer *renderer)
{
	ui_objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT));
	ui_objects.emplace_back(new Button(renderer, 212, 222, 375, 50, STR_BTN_SINGLEPLAYER, true));
	ui_objects.emplace_back(new Button(renderer, 212, 285, 375, 50, STR_BTN_MULTIPLAYER));
	ui_objects.emplace_back(new Button(renderer, 212, 347, 375, 50, STR_BTN_HELP));
	ui_objects.emplace_back(new Button(renderer, 212, 410, 375, 50, STR_BTN_EDIT));
	ui_objects.emplace_back(new Button(renderer, 212, 472, 375, 50, STR_BTN_EXIT));
	// FIXME (tm) gets truncated by resource handling in res.h (ascii, unicode stuff)
	ui_objects.emplace_back(new Text(renderer, WIDTH / 2, 542, STR_MAIN_COPY1, CENTER));
	// FIXME (copy) and (p) before this line
	ui_objects.emplace_back(new Text(renderer, WIDTH / 2, 561, STR_MAIN_COPY2, CENTER));
	ui_objects.emplace_back(new Text(renderer, WIDTH / 2, 578, STR_MAIN_COPY3, CENTER));
}

void ui_free(void)
{
}

void display(SDL_Renderer *renderer)
{
	for (auto x : ui_objects) {
		auto o = x.get();
		o->draw(renderer);
	}
}

}
