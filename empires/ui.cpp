/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cstdio>

#include <string>
#include <memory>
#include <vector>

#include "gfx.h"

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

class Text final : public UI {
	std::string str;
	SDL_Surface *surf;
	SDL_Texture *tex;
public:
	Text(SDL_Renderer *renderer, int x, int y, const std::string &str) : UI(x, y), str(str) {
		SDL_Color col = {255, 255, 255, SDL_ALPHA_OPAQUE};
		surf = TTF_RenderText_Solid(fnt_default, str.c_str(), col);
		tex = SDL_CreateTextureFromSurface(renderer, surf);
		// Update new bounds
		w = surf->w;
		h = surf->h;
	}

	~Text() {
		SDL_DestroyTexture(tex);
		SDL_FreeSurface(surf);
	}

	void draw(SDL_Renderer *renderer) const {
		SDL_Rect pos = {x, y, (int)w, (int)h};
		SDL_RenderCopy(renderer, tex, NULL, &pos);
	}
};

class Button final : public UI {
public:
	Button(int x, int y, unsigned w=374, unsigned h=49)
		: UI(x, y, w, h) {}

	void draw(SDL_Renderer *renderer) const {
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

std::vector<std::shared_ptr<UI>> ui_objects;

extern "C"
{

void ui_init(SDL_Renderer *renderer)
{
	ui_objects.emplace_back(new Button(212, 222));
	ui_objects.emplace_back(new Button(212, 285));
	ui_objects.emplace_back(new Button(212, 347));
	ui_objects.emplace_back(new Button(212, 410));
	ui_objects.emplace_back(new Button(212, 472));
	ui_objects.emplace_back(new Text(renderer, WIDTH / 2, 542, "burps"));
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
