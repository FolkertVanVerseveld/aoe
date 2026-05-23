#include "fullscreenmenu.hpp"

#include <cassert>

namespace aoe {
namespace ui {

VerticalGroup::VerticalGroup(MenuButton *buttons, unsigned buttonCount)
	: buttons(buttons), buttonCount(buttonCount)
	, activated(false), selected(0)
{
	assert(buttons);
	assert(buttonCount);
	buttons[selected].state |= (unsigned)MenuButtonState::selected;
}

void VerticalGroup::reshape(ImGuiViewport *vp) {
	for (unsigned i = 0; i < buttonCount; ++i)
		buttons[i].reshape(vp);
}

void VerticalGroup::show(Frame &f, BackgroundColors &col) {
	for (unsigned i = 0, n = buttonCount; i < n; ++i)
		buttons[i].show(f, col);
}

}
}