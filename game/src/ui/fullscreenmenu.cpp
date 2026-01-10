#include "fullscreenmenu.hpp"
#include "../engine/audio.hpp"

#include <cassert>

namespace aoe {
namespace ui {

void FullscreenMenu::reshape(ImGuiViewport *vp) {
	for (unsigned i = 0; i < buttonCount; ++i)
		buttons[i].reshape(vp);
}

FullscreenMenu::FullscreenMenu(unsigned menuState, MenuButton *buttons, unsigned buttonCount,
	void (*fn)(Frame &, Audio &, Assets &))
	: menuState(menuState), buttons(buttons), buttonCount(buttonCount), selected(0), draw(fn)
{
	assert(buttonCount);
	buttons[selected].state |= (unsigned)MenuButtonState::selected;
}

static inline bool PointInMenuButton(const MenuButton *btn, int mx, int my)
{
	assert(btn);
	SDL_FPoint pt{ mx, my };
	SDL_FRect rect{ btn->x0, btn->y0, btn->x1 - btn->x0, btn->y1 - btn->y0 };

	return SDL_PointInFRect(&pt, &rect);
}

void FullscreenMenu::mouse_down(int mx, int my, Audio &sfx) {
	unsigned mask = (unsigned)MenuButtonState::active | (unsigned)MenuButtonState::selected;
	bool activated = false;

	for (unsigned i = 0; i < buttonCount; ++i) {
		MenuButton *btn = &buttons[i];

		if (!activated && PointInMenuButton(btn, mx, my)) {
			activated = true; // only one button can be activated
			btn->state |= mask;
			selected = i;
			sfx.play_sfx(SfxId::ui_click);
		} else {
			btn->state &= ~mask;
		}
	}
}

void FullscreenMenu::mouse_up(int mx, int my, MenuState state) {
	if (selected >= buttonCount)
		return;

	MenuButton *btn = &buttons[selected];
	unsigned mselected = (unsigned)MenuButtonState::selected;
	unsigned mactive = (unsigned)MenuButtonState::active;

	if (PointInMenuButton(btn, mx, my)) {
		btn->state = (btn->state | mselected) & ~mactive;
		MenuButtonActivate(state, selected);
	} else {
		btn->state &= ~mactive;
	}
}

void FullscreenMenu::kbp_down(GameKey key) {
	unsigned old = selected;

	if (key == GameKey::ui_prev)
		dec(selected);
	else if (key == GameKey::ui_next)
		inc(selected, buttonCount - 1);

	unsigned mask = (unsigned)MenuButtonState::selected;

	buttons[old].state &= ~mask;
	buttons[selected].state |= mask;
}

}
}