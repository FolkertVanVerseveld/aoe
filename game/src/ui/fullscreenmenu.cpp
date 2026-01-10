#include "fullscreenmenu.hpp"
#include "../engine/audio.hpp"

#include <cassert>

namespace aoe {
namespace ui {

void FullscreenMenu::reshape(ImGuiViewport *vp) {
	for (unsigned i = 0; i < buttonCount; ++i)
		buttons[i].reshape(vp);
}

FullscreenMenu::FullscreenMenu(MenuState menuState, MenuButton *buttons, unsigned buttonCount,
	void (*fn)(Frame &, Audio &, Assets &))
	: menuState(menuState), buttons(buttons), buttonCount(buttonCount), activated(false), selected(0), draw(fn)
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
	activated = false;

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

	if (!activated)
		buttons[selected].state |= (unsigned)MenuButtonState::selected;
}

void FullscreenMenu::mouse_up(int mx, int my) {
	if (!activated || selected >= buttonCount)
		return;

	MenuButton *btn = &buttons[selected];
	unsigned mselected = (unsigned)MenuButtonState::selected;
	unsigned mactive = (unsigned)MenuButtonState::active;

	activated = false;

	if (PointInMenuButton(btn, mx, my)) {
		btn->state = (btn->state | mselected) & ~mactive;
		MenuButtonActivate(menuState, selected);
	} else {
		btn->state &= ~mactive;
	}
}

void FullscreenMenu::key_tapped(GameKey key) {
	if (key == GameKey::ui_select) {
		buttons[selected].state &= ~(unsigned)MenuButtonState::active;
		MenuButtonActivate(menuState, selected);
	}
}

void FullscreenMenu::key_down(GameKey key, KeyboardController &keyctl, Audio &sfx) {
	unsigned old = selected;

	if (keyctl.is_down(GameKey::ui_select)) {
		// only do this if we just selected
		if (key == GameKey::ui_select) {
			buttons[selected].state |= (unsigned)MenuButtonState::active;
			sfx.play_sfx(SfxId::ui_click);
		}
		return;
	}

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