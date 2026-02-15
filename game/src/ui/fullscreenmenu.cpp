#include "fullscreenmenu.hpp"
#include "../engine/audio.hpp"

#include <cassert>

namespace aoe {
namespace ui {

static inline void dec(unsigned &v, unsigned min=0)
{
	if (v > min)
		--v;
	else
		v = min;
}

static inline void inc(unsigned &v, unsigned max)
{
	if (v < max)
		++v;
	else
		v = max;
}

MenuButton quitButton({ true, 4, 24, 32 }, "X");

void FullscreenMenu::reshape(ImGuiViewport *vp) {
	for (unsigned i = 0; i < buttonCount; ++i)
		buttons[i].reshape(vp);

	quitButton.reshape(vp);
}

FullscreenMenu::FullscreenMenu(MenuState menuState, MenuButton *buttons, unsigned buttonCount,
	const char *frameTitle, const char *title, void (*fnActivate)(unsigned idx))
	: menuState(menuState), buttons(buttons), buttonCount(buttonCount), activated(false), selected(0)
	, frameTitle(frameTitle), title(title), fnActivate(fnActivate)
{
	assert(buttonCount);
	assert(fnActivate);
	buttons[selected].state |= (unsigned)MenuButtonState::selected;
}

static inline bool PointInMenuButton(const MenuButton *btn, int mx, int my)
{
	assert(btn);
	SDL_FPoint pt{ mx, my };

	return SDL_PointInFRect(&pt, &btn->bnds);
}

void FullscreenMenu::mouse_down(int mx, int my, Audio &sfx) {
	const unsigned mactive = (unsigned)MenuButtonState::active;
	const unsigned mselected = (unsigned)MenuButtonState::selected;
	unsigned mask = mactive | mselected;
	activated = false;

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my)) {
		quitButton.state |= mactive;
		sfx.play_sfx(SfxId::ui_click);
		return;
	}

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
		buttons[selected].state |= mselected;
}

void FullscreenMenu::mouse_up(int mx, int my) {
	const unsigned mselected = (unsigned)MenuButtonState::selected;
	const unsigned mactive = (unsigned)MenuButtonState::active;

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my) && (quitButton.state & mactive))
		throw 0;

	quitButton.state &= ~mactive;

	if (!activated || selected >= buttonCount)
		return;

	MenuButton *btn = &buttons[selected];
	activated = false;

	if (PointInMenuButton(btn, mx, my)) {
		btn->state = (btn->state | mselected) & ~mactive;
		fnActivate(selected);
	} else {
		btn->state &= ~mactive;
	}
}

void FullscreenMenu::key_tapped(GameKey key) {
	if (key == GameKey::ui_select) {
		buttons[selected].state &= ~(unsigned)MenuButtonState::active;
		fnActivate(selected);
	} else if (key == GameKey::ui_back) {
		fnActivate(-1);
	}
}

void FullscreenMenu::key_down(GameKey key, KeyboardController &keyctl, Audio &sfx) {
	unsigned old = selected;

	if (keyctl.is_down(GameKey::ui_back)) {
		if (key == GameKey::ui_back)
			sfx.play_sfx(SfxId::ui_click);
		return;
	}

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