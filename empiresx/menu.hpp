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
	std::vector<ui::InputField*> ui_inputs;
public:
	Text title;
	BackgroundSettings bkg;
	Palette pal;
	ui::Border border;
	Animation anim_bkg[lgy_screen_modes];
	bool enhanced;

	static constexpr unsigned show_border = 0x01;
	static constexpr unsigned show_title = 0x02;
	static constexpr unsigned show_background = 0x04;

	Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, bool enhanced=false);
	virtual ~Menu() {}

	void add_btn(ui::Button *btn);
	void add_field(ui::InputField *field);
	void go_to(Menu *menu);

	virtual void resize(ConfigScreenMode old_mode, ConfigScreenMode mode);

	virtual void mousedown(SDL_MouseButtonEvent &ev);
	virtual void mouseup(SDL_MouseButtonEvent &ev);

	virtual bool keydown(int ch);
	virtual bool keyup(int ch);

	virtual void idle() {}
	virtual void paint();

	void paint_details(unsigned options);
};

class Navigator final {
	SimpleRender &r;
	// not using stack since we want to address menus in the middle as well
	std::vector<std::unique_ptr<Menu>> trace;
	// remembers old menus that have to be removed the next game tick
	std::vector<std::unique_ptr<Menu>> to_purge;
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
