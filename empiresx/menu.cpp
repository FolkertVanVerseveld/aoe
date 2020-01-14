#include "menu.hpp"

namespace genie {

Menu::Menu(SimpleRender& r, Font& f, const std::string& s, SDL_Color fg) : r(r), f(f), title(r, f, s, fg) {}

void Menu::paint() {
	r.clear();

	title.paint(r, 40, 40);
}

std::unique_ptr<Navigator> nav;

}