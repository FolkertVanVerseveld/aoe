#pragma once

#include "render.hpp"
#include "font.hpp"
#include "drs.hpp"
#include "ui.hpp"

#include <memory>
#include <string>
#include <stack>
#include <vector>

namespace genie {

class Menu {
protected:
	SimpleRender &r;
public:
	Text title;
	Background bkg;
	Palette pal;
	ui::Border border;
	Animation anim_bkg[3];

	Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg);
	virtual ~Menu() {}

	void resize(const SDL_Rect &old_abs, const SDL_Rect &cur_abs, const SDL_Rect &old_rel, const SDL_Rect &cur_rel);

	virtual void keydown(int ch) {}

	virtual void idle() {};
	virtual void paint();
};

class Navigator final {
	SimpleRender &r;
	// not using stack since we want to address menus in the middle as well
	std::vector<std::unique_ptr<Menu>> trace;
	Menu *top;
public:
	Navigator(SimpleRender &r);

	void mainloop();
	void resize(const SDL_Rect &old_abs, const SDL_Rect &cur_abs, const SDL_Rect &old_rel, const SDL_Rect &cur_rel);
	void go_to(Menu *m);
	void quit(unsigned count = 0);
};

extern std::unique_ptr<Navigator> nav;

}