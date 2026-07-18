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

static inline void inc_to(unsigned &v, unsigned max)
{
	if (v < max)
		++v;
	else
		v = max;
}

static inline void inc(unsigned &v, unsigned max)
{
	if (v < max)
		++v;
	else
		v = 0;
}

MenuButton quitButton({ true, 6, 24, 32 }, "X", (unsigned)MenuButtonState::hidden);

void FullscreenMenu::reshape(ImGuiViewport *vp) {
	orthogonal->reshape(vp);
	quitButton.reshape(vp);
}

FullscreenMenu::FullscreenMenu(MenuState menuState, OrthogonalGroup &orthogonal,
	const char *frameTitle, const char *title, void (*fnActivate)(unsigned idx))
	: menuState(menuState), orthogonal(&orthogonal), selecting(SelectMode::wait)
	, frameTitle(frameTitle), title(title), fnActivate(fnActivate)
{
	assert(fnActivate);
}

static inline bool PointInFRect(const SDL_FPoint &p, const SDL_FRect &r)
{
	return p.x >= r.x && p.x < r.x + r.w && p.y >= r.y && p.y < r.y + r.h;
}

static inline bool PointInMenuButton(const MenuButton *btn, int mx, int my)
{
	assert(btn);
	SDL_FPoint pt{ mx, my };

	return PointInFRect(pt, btn->bnds);
}

void FullscreenMenu::mouse_down(int mx, int my, Audio &sfx) {
	if (selecting != SelectMode::wait)
		return;

	const unsigned mactive = (unsigned)MenuButtonState::active;
	const unsigned mselected = (unsigned)MenuButtonState::selected;
	unsigned mask = mactive | mselected;
	orthogonal->activated = false;

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my) && !quitButton.is_hidden()) {
		quitButton.state |= mactive;
		selecting = SelectMode::mouse;
		sfx.play_sfx(SfxId::ui_click);
		return;
	}

	for (unsigned i = 0; i < orthogonal->buttonCount; ++i) {
		MenuButton *btn = &orthogonal->buttons[i];

		if (!orthogonal->activated && !btn->is_hidden() && PointInMenuButton(btn, mx, my)) {
			orthogonal->activated = true; // only one button can be activated
			btn->state |= mask;
			orthogonal->selected = i;
			selecting = SelectMode::mouse;
			sfx.play_sfx(SfxId::ui_click);
		} else {
			btn->state &= ~mask;
		}
	}

	if (!orthogonal->activated) {
		orthogonal->buttons[orthogonal->selected].state |= mselected;
		selecting = SelectMode::mouse;
	}
}

void FullscreenMenu::mouse_up(int mx, int my) {
	if (selecting != SelectMode::mouse)
		return;

	selecting = SelectMode::wait;

	const unsigned mselected = (unsigned)MenuButtonState::selected;
	const unsigned mactive = (unsigned)MenuButtonState::active;

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my) && !quitButton.is_hidden() && (quitButton.state & mactive))
		throw 0;

	quitButton.state &= ~mactive;

	if (!orthogonal->activated || orthogonal->selected >= orthogonal->buttonCount)
		return;

	MenuButton *btn = &orthogonal->buttons[orthogonal->selected];
	orthogonal->activated = false;

	if (PointInMenuButton(btn, mx, my) && !btn->is_hidden()) {
		btn->state = (btn->state | mselected) & ~mactive;
		fnActivate(orthogonal->selected);
	} else {
		btn->state &= ~mactive;
	}
}

void FullscreenMenu::key_tapped(GameKey key) {
	if (selecting != SelectMode::keyboard)
		return;

	if (key == GameKey::ui_select || key == GameKey::ui_back) {
		selecting = SelectMode::wait;
		unsigned idx = orthogonal->selected;
		orthogonal->buttons[idx].state &= ~(unsigned)MenuButtonState::active;

		if (key == GameKey::ui_back)
			idx = -1;

		fnActivate(idx);
	}
}

void FullscreenMenu::key_down(GameKey key, KeyboardController &keyctl, Audio &sfx) {
	if (selecting != SelectMode::wait)
		return;

	if (key == GameKey::ui_back || key == GameKey::ui_select) {
		selecting = SelectMode::keyboard;
		sfx.play_sfx(SfxId::ui_click);

		if (key == GameKey::ui_select)
			orthogonal->buttons[orthogonal->selected].state |= (unsigned)MenuButtonState::active;

		return;
	}

	unsigned old = orthogonal->selected;

	// TODO add groups to tabulate through

	if (key == GameKey::ui_prev)
		dec(orthogonal->selected);
	else if (key == GameKey::ui_next)
		inc_to(orthogonal->selected, orthogonal->buttonCount - 1);
	else if (key == GameKey::ui_tabulate)
		inc(orthogonal->selected, orthogonal->buttonCount - 1);

	if (orthogonal->selected != old) {
		unsigned mask = (unsigned)MenuButtonState::selected;

		orthogonal->buttons[old].state &= ~mask;
		orthogonal->buttons[orthogonal->selected].state |= mask;
	}
}

void FullscreenMenu::drawButtons(Frame &f, BackgroundColors &col, Assets &ass, Audio &sfx) {
	orthogonal->show(f, col);
	quitButton.show(f, col);
}

}
}
