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

MenuButton quitButton({ true, 6, 24, 32 }, "X");

void FullscreenMenu::reshape(ImGuiViewport *vp) {
	vertical.reshape(vp);
	quitButton.reshape(vp);
}

FullscreenMenu::FullscreenMenu(MenuState menuState, MenuButton *buttons, unsigned buttonCount,
	const char *frameTitle, const char *title, void (*fnActivate)(unsigned idx))
	: menuState(menuState), vertical(buttons, buttonCount)
	, frameTitle(frameTitle), title(title), fnActivate(fnActivate)
{
	assert(fnActivate);
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
	vertical.activated = false;

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my)) {
		quitButton.state |= mactive;
		sfx.play_sfx(SfxId::ui_click);
		return;
	}

	for (unsigned i = 0; i < vertical.buttonCount; ++i) {
		MenuButton *btn = &vertical.buttons[i];

		if (!vertical.activated && PointInMenuButton(btn, mx, my)) {
			vertical.activated = true; // only one button can be activated
			btn->state |= mask;
			vertical.selected = i;
			sfx.play_sfx(SfxId::ui_click);
		} else {
			btn->state &= ~mask;
		}
	}

	if (!vertical.activated)
		vertical.buttons[vertical.selected].state |= mselected;
}

void FullscreenMenu::mouse_up(int mx, int my) {
	const unsigned mselected = (unsigned)MenuButtonState::selected;
	const unsigned mactive = (unsigned)MenuButtonState::active;

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my) && (quitButton.state & mactive))
		throw 0;

	quitButton.state &= ~mactive;

	if (!vertical.activated || vertical.selected >= vertical.buttonCount)
		return;

	MenuButton *btn = &vertical.buttons[vertical.selected];
	vertical.activated = false;

	if (PointInMenuButton(btn, mx, my)) {
		btn->state = (btn->state | mselected) & ~mactive;
		fnActivate(vertical.selected);
	} else {
		btn->state &= ~mactive;
	}
}

void FullscreenMenu::key_tapped(GameKey key) {
	if (key == GameKey::ui_select) {
		vertical.buttons[vertical.selected].state &= ~(unsigned)MenuButtonState::active;
		fnActivate(vertical.selected);
	} else if (key == GameKey::ui_back) {
		fnActivate(-1);
	}
}

void FullscreenMenu::key_down(GameKey key, KeyboardController &keyctl, Audio &sfx) {
	unsigned old = vertical.selected;

	if (keyctl.is_down(GameKey::ui_back)) {
		if (key == GameKey::ui_back)
			sfx.play_sfx(SfxId::ui_click);
		return;
	}

	if (keyctl.is_down(GameKey::ui_select)) {
		// only do this if we just selected
		if (key == GameKey::ui_select) {
			vertical.buttons[vertical.selected].state |= (unsigned)MenuButtonState::active;
			sfx.play_sfx(SfxId::ui_click);
		}
		return;
	}

	if (key == GameKey::ui_prev)
		dec(vertical.selected);
	else if (key == GameKey::ui_next)
		inc(vertical.selected, vertical.buttonCount - 1);

	unsigned mask = (unsigned)MenuButtonState::selected;

	vertical.buttons[old].state &= ~mask;
	vertical.buttons[vertical.selected].state |= mask;
}

void FullscreenMenu::drawButtons(Frame &f, BackgroundColors &col, Assets &ass, Audio &sfx) {
	for (unsigned i = 0, n = vertical.buttonCount; i < n; ++i)
		vertical.buttons[i].show(f, sfx, col);

	quitButton.show(f, sfx, col);
}

}
}