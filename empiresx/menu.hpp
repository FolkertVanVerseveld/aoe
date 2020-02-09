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
	std::vector<std::unique_ptr<ui::DynamicUI>> ui_objs;
	std::vector<ui::Button*> ui_focus;
public:
	Text title;
	BackgroundSettings bkg;
	Palette pal;
	ui::Border border;
	Animation anim_bkg[lgy_screen_modes];
	bool enhanced;

	Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, bool enhanced=false);
	virtual ~Menu() {}

	void add_btn(ui::Button *btn);
	void go_to(Menu *menu);

	virtual void resize(ConfigScreenMode old_mode, ConfigScreenMode mode);

	virtual void mousedown(SDL_MouseButtonEvent &ev);
	virtual void mouseup(SDL_MouseButtonEvent &ev);

	virtual void keydown(int ch) {}
	virtual void keyup(int ch) {}

	virtual void idle() {}
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
	void resize(ConfigScreenMode old_mode, ConfigScreenMode mode);
	void go_to(Menu *m);
	void quit(unsigned count = 0);
};

extern std::unique_ptr<Navigator> nav;

}