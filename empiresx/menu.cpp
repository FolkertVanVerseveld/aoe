#include "menu.hpp"

#include <cassert>

#include "engine.hpp"

namespace genie {

Menu::Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, bool enhanced)
	: r(r), title(r, f, s, fg)
	, bkg(eng->assets->open_bkg((res_id)id)), pal(eng->assets->open_pal(bkg.pal))
	, border(r.dim.rel_bnds, pal, bkg)
	, anim_bkg{eng->assets->open_slp(pal, bkg.bmp[0]), eng->assets->open_slp(pal, bkg.bmp[1]), eng->assets->open_slp(pal, bkg.bmp[2])}
	, enhanced(enhanced)
{
	resize(r.dim, r.dim);
}

void Menu::paint() {
	r.color({0, 0, 0, SDL_ALPHA_OPAQUE});
	r.clear();

	int index = 0;
	SDL_Rect to(enhanced ? r.dim.rel_bnds : r.dim.lgy_orig);

	if (enhanced) {
		if (r.dim.rel_bnds.w > lgy_dim[1].w)
			index = 2;
		else if (r.dim.rel_bnds.w > lgy_dim[0].w)
			index = 1;
	} else {
		r.legvp();

		if (r.dim.lgy_bnds.w > lgy_dim[1].w)
			index = 2;
		else if (r.dim.lgy_bnds.w > lgy_dim[0].w)
			index = 1;
	}

	anim_bkg[index].subimage(0).draw_stretch(r, to);

	border.paint(r);
	title.paint(r, 40, 40);
}

void Menu::resize(const Dimensions &old_dim, const Dimensions &dim) {
	border.resize(enhanced ? dim.rel_bnds : dim.lgy_orig);
}

std::unique_ptr<Navigator> nav;

void Navigator::mainloop() {
	while (1) {
		SDL_Event ev;

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
				switch (ev.key.keysym.sym) {
				case SDLK_F11:
					eng->w->toggleFullscreen();
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

void Navigator::resize(const Dimensions &old_dim, const Dimensions &dim) {
	for (auto &x : trace)
		x->resize(old_dim, dim);
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