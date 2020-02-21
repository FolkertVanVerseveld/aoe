#include "menu.hpp"

#include <cassert>

#include "engine.hpp"

namespace genie {

Menu::Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, bool enhanced)
	: r(r), ui_objs(), ui_focus(), ui_inputs(), title(r, f, s, fg)
	, bkg(eng->assets->open_bkg((res_id)id)), pal(eng->assets->open_pal(bkg.pal))
	, border(scr_dim, eng->w->mode(), pal, bkg, ui::BorderType::background, enhanced)
	, anim_bkg{eng->assets->open_slp(pal, bkg.bmp[0]), eng->assets->open_slp(pal, bkg.bmp[1]), eng->assets->open_slp(pal, bkg.bmp[2])}
	, enhanced(enhanced)
{
	r.legvp(!enhanced);
	resize(r.mode, r.mode);
}

void Menu::add_btn(ui::Button *btn) {
	ui_objs.emplace_back(btn);
	ui_focus.emplace_back(btn);
}

void Menu::add_field(ui::InputField *field) {
	ui_objs.emplace_back(field);
	ui_inputs.emplace_back(field);
}

void Menu::paint_details(unsigned options) {
	r.color({0, 0, 0, SDL_ALPHA_OPAQUE});
	r.clear();

	int index = 0;
	SDL_Rect to(enhanced ? r.dim.rel_bnds : r.dim.lgy_orig), ref(enhanced ? r.dim.rel_bnds : r.dim.lgy_bnds);

	r.legvp(!enhanced);

	if (ref.w > lgy_dim[1].w)
		index = 2;
	else if (ref.w > lgy_dim[0].w)
		index = 1;

	anim_bkg[index].subimage(0).draw_stretch(r, to);

	if (options & show_border)
		border.paint(r);
	if (options & show_title)
		title.paint(r, (ref.w - title.tex().width) / 2, 40);

	for (auto &x : ui_objs)
		x->paint(r);
}

void Menu::paint() {
	paint_details(Menu::show_border | Menu::show_title);
}

void Menu::mousedown(SDL_MouseButtonEvent &ev) {
	r.legvp(!enhanced);

	if (!enhanced && eng->w->mode() == ConfigScreenMode::MODE_FULLSCREEN) {
		ev.x -= r.offset.x;
		ev.y -= r.offset.y;
	}

	if (ui_objs.empty())
		return;

	for (auto &x : ui_focus)
		x->press(x->collides(ev.x, ev.y));
}

void Menu::mouseup(SDL_MouseButtonEvent &ev) {
	r.legvp(!enhanced);

	if (!enhanced && eng->w->mode() == ConfigScreenMode::MODE_FULLSCREEN) {
		ev.x -= r.offset.x;
		ev.y -= r.offset.y;
	}

	if (ui_objs.empty())
		return;

	for (auto &x : ui_focus)
		x->press(false);

	for (auto &x : ui_inputs)
		x->focus(x->collides(ev.x, ev.y));
}

void Menu::go_to(Menu *menu) {
	for (auto &x : ui_focus)
		x->press(false);

	nav->go_to(menu);
}

void Menu::resize(ConfigScreenMode old_mode, ConfigScreenMode mode) {
	r.legvp(!enhanced);
	border.resize(old_mode, mode);

	for (auto &x : ui_objs)
		x->resize(old_mode, mode);
}

bool Menu::keydown(int ch) {
	for (auto &x : ui_inputs)
		if (x->keydown(ch))
			return true;

	return false;
}

bool Menu::keyup(int ch) {
	for (auto &x : ui_inputs)
		if (x->keyup(ch))
			return true;

	return false;
}

std::unique_ptr<Navigator> nav;

void Navigator::mainloop() {
	while (1) {
		SDL_Event ev;

		if (to_purge.size()) {
			to_purge.clear();
			top = trace.size() ? trace[trace.size() - 1].get() : nullptr;
		}

		if (!top)
			return;

		// FIXME display events are instantly completed in fullscreen alt-tabbed mode, maxing CPU usage

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				switch (ev.key.keysym.sym) {
				case SDLK_F11:
					break;
				default:
					top->keydown(ev.key.keysym.sym);
					break;
				}
				break;
			case SDL_KEYUP:
				switch (ev.key.keysym.sym) {
				case SDLK_F11:
					eng->w->toggleFullscreen();
					break;
				default:
					top->keyup(ev.key.keysym.sym);
					break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				top->mousedown(ev.button);
				break;
			case SDL_MOUSEBUTTONUP:
				top->mouseup(ev.button);
				break;
			}
		}

		top->idle();

		std::lock_guard<std::recursive_mutex> lock(eng->sdl.mut);
		top->paint();
		r.paint();
	}
}

void Navigator::resize(ConfigScreenMode old_mode, ConfigScreenMode mode) {
	for (auto &x : trace)
		x->resize(old_mode, mode);
}

void Navigator::go_to(Menu *m) {
	assert(m);
	trace.emplace_back(top = m);
}

// forward popped menus to list to keep one event loop tick
void Navigator::quit(unsigned count) {
	if (!count || count >= trace.size()) {
		for (auto &x : trace) {
			to_purge.emplace_back(x.release());
			trace.pop_back();
		}
		return;
	}

	for (unsigned i = 0; i < count; ++i) {
		to_purge.emplace_back(trace[trace.size() - 1].release());
		trace.pop_back();
	}
}

}
