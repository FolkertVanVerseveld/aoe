#include "menu.hpp"

#include "engine.hpp"

namespace genie {

Menu::Menu(res_id dlgid, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg) : r(r), title(r, f, s, fg), bkg(eng->assets->open_bkg(dlgid)), pal(eng->assets->open_pal(bkg.pal)), border(r.rel_bnds, pal, bkg) {}

void Menu::paint() {
	r.color({0, 0, 0, SDL_ALPHA_OPAQUE});
	r.clear();

	border.paint(r);

	title.paint(r, 40, 40);
}

std::unique_ptr<Navigator> nav;

}