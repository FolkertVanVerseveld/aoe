#include "fullscreenmenu.hpp"

#include <cassert>

namespace aoe {
namespace ui {

OrthogonalGroup::OrthogonalGroup(MenuButton *buttons, unsigned buttonCount, unsigned selected)
	: buttons(buttons), buttonCount(buttonCount)
	, activated(false), selected(selected)
{
	assert(buttons);
	assert(buttonCount);
	assert(selected < buttonCount);
	buttons[selected].state |= (unsigned)MenuButtonState::selected;
}

void OrthogonalGroup::reshape(ImGuiViewport *vp) {
	for (unsigned i = 0; i < buttonCount; ++i)
		buttons[i].reshape(vp);
}

void OrthogonalGroup::show(Frame &f, BackgroundColors &col) {
	for (unsigned i = 0, n = buttonCount; i < n; ++i)
		buttons[i].show(f, col);
}

}
}