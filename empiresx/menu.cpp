#include "menu.hpp"

#include <cassert>

#include "engine.hpp"

namespace genie {

Menu::Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, bool enhanced)
	: r(r), title(r, f, s, fg)
	, bkg(eng->assets->open_bkg((res_id)id)), pal(eng->assets->open_pal(bkg.pal))
	, border(scr_dim, eng->w->mode(), pal, bkg, ui::BorderType::background, enhanced)
	, anim_bkg{eng->assets->open_slp(pal, bkg.bmp[0]), eng->assets->open_slp(pal, bkg.bmp[1]), eng->assets->open_slp(pal, bkg.bmp[2])}
	, enhanced(enhanced)
{
	r.legvp(!enhanced);
	resize(r.mode, r.mode);
}

void Menu::paint() {
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

	border.paint(r);
	title.paint(r, 40, 40);

	for (auto &x : ui_objs)
		x->paint(r);
}

void Menu::resize(ConfigScreenMode old_mode, ConfigScreenMode mode) {
	border.resize(old_mode, mode);

	for (auto &x : ui_objs)
		x->resize(old_mode, mode);
}

std::unique_ptr<Navigator> nav;

void Navigator::mainloop() {
	while (1) {
		SDL_Event ev;

		// FIXME display events are instantly completed in fullscreen alt-tabbed mode, maxing CPU usage

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				return;
			case SDL_KEYDOWN:
				if (!top) {
					quit();
					return;
				}

				switch (ev.key.keysym.sym) {
				case SDLK_F11:
					break;
				default:
					top->keydown(ev.key.keysym.sym);
					break;
				}
				break;
			case SDL_KEYUP:
				if (!top) {
					quit();
					return;
				}

				switch (ev.key.keysym.sym) {
				case SDLK_F11:
					eng->w->toggleFullscreen();
					break;
				default:
					top->keyup(ev.key.keysym.sym);
					break;
				}
				break;
			}
		}

		if (!top) {
			quit();
			return;
		}
		top->idle();

		if (!top) {
			quit();
			return;
		}
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

void Navigator::quit(unsigned count) {
	if (!count || count >= trace.size()) {
		trace.clear();
		top = nullptr;
		return;
	}

	trace.erase(trace.end() - count);
	top = trace[trace.size() - 1].get();
}

}