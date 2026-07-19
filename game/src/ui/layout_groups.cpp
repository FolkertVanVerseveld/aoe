#include "fullscreenmenu.hpp"

#include <cassert>

namespace aoe {
namespace ui {

OrthogonalGroup::OrthogonalGroup(MenuButton *buttons, unsigned buttonCount, void (*fnActivate)(unsigned), unsigned selected)
	: buttons(buttons), buttonCount(buttonCount)
	, activated(false), selected(selected)
	, fnActivate(fnActivate)
{
	assert(buttons && buttonCount);
	assert(fnActivate);

	if (selected < buttonCount)
		buttons[selected].state |= (unsigned)MenuButtonState::selected;
	else
		selected = 0;
}

void OrthogonalGroup::reshape(ImGuiViewport *vp) {
	for (unsigned i = 0, n = buttonCount; i < n; ++i)
		buttons[i].reshape(vp);
}

void OrthogonalGroup::show(Frame &f, BackgroundColors &col) {
	for (unsigned i = 0, n = buttonCount; i < n; ++i)
		buttons[i].show(f, col);
}

void OrthogonalGroup::unfocus() {
	buttons[selected].state &= ~((unsigned)MenuButtonState::active | (unsigned)MenuButtonState::selected);
}

void OrthogonalGroup::focus(unsigned index) {
	// deselect current
	buttons[selected].state &= ~((unsigned)MenuButtonState::active | (unsigned)MenuButtonState::selected);

	selected = index % buttonCount;
	buttons[selected].state |= (unsigned)MenuButtonState::selected;
}

}
}