#include "menu.hpp"

#include <cassert>

#include "engine.hpp"

namespace genie {

Menu::Menu(MenuId id, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg)
	: r(r), title(r, f, s, fg)
	, bkg(eng->assets->open_bkg((res_id)id)), pal(eng->assets->open_pal(bkg.pal))
	, border(r.rel_bnds, pal, bkg)
	, anim_bkg{eng->assets->open_slp(pal, bkg.bmp[0]), eng->assets->open_slp(pal, bkg.bmp[1]), eng->assets->open_slp(pal, bkg.bmp[2])} {}

void Menu::paint() {
	r.color({0, 0, 0, SDL_ALPHA_OPAQUE});
	r.clear();

	switch (r.rel_bnds.w) {
	case 640:
		anim_bkg[0].subimage(0).draw(r, 0, 0);
		break;
	case 800:
		anim_bkg[1].subimage(0).draw(r, 0, 0);
		break;
	default:
		anim_bkg[2].subimage(0).draw(r, 0, 0);
		break;
	}

	border.paint(r);

	title.paint(r, 40, 40);
}

void Menu::resize(const SDL_Rect &old_abs, const SDL_Rect &cur_abs, const SDL_Rect &old_rel, const SDL_Rect &cur_rel) {
	border.resize(cur_rel);
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
				case SDLK_F4:
					break;
				default:
					top->keydown(ev.key.keysym.sym);
					break;
				}
				break;
			case SDL_KEYUP:
				switch (ev.key.keysym.sym) {
				case SDLK_F4:
					eng->nextmode();
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

void Navigator::resize(const SDL_Rect &old_abs, const SDL_Rect &cur_abs, const SDL_Rect &old_rel, const SDL_Rect &cur_rel) {
	for (auto &x : trace)
		x->resize(old_abs, cur_abs, old_rel, cur_rel);
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