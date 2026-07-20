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
	for (unsigned i = 0; i < ogCount; ++i)
		orthogonal[i].reshape(vp);

	quitButton.reshape(vp);

	if (labelCount) {
		MenuLabel *lbl = labels;
		for (unsigned i = 0; i < labelCount; ++i, ++lbl)
			lbl->reshape(vp);
	}
}

FullscreenMenu::FullscreenMenu(MenuState menuState, OrthogonalGroup &orthogonal,
	const char *frameTitle, const char *title,
	MenuLabel *labels, unsigned labelCount)
	: FullscreenMenu(menuState, &orthogonal, 1, frameTitle, title, labels, labelCount) {}

FullscreenMenu::FullscreenMenu(MenuState menuState, OrthogonalGroup *orthogonal,
	unsigned ogCount,
	const char *frameTitle, const char *title,
	MenuLabel *labels, unsigned labelCount)
	: menuState(menuState), orthogonal(orthogonal), ogIndex(0), ogCount(ogCount), selecting(SelectMode::wait)
	, frameTitle(frameTitle), title(title)
	, labels(labels), labelCount(labelCount)
{
	assert(!labelCount || (labelCount && labels));

	for (unsigned i = 1; i < ogCount; ++i)
		orthogonal[i].unfocus();
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

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my) && !quitButton.is_hidden()) {
		quitButton.state |= mactive;
		selecting = SelectMode::mouse;
		sfx.play_sfx(SfxId::ui_click);
		return;
	}

	for (unsigned j = 0; j < ogCount; ++j) {
		OrthogonalGroup &og = orthogonal[j];
		og.activated = false;

		for (unsigned i = 0; i < og.buttonCount; ++i) {
			MenuButton *btn = &og.buttons[i];

			if (!og.activated && !btn->is_hidden() && PointInMenuButton(btn, mx, my)) {
				og.activated = true; // only one button can be activated
				btn->state |= mask;
				og.selected = i;
				selecting = SelectMode::mouse;
				sfx.play_sfx(SfxId::ui_click);
				ogIndex = j;
			} else {
				btn->state &= ~mask;
			}
		}

		if (!og.activated) {
			og.buttons[og.selected].state |= mselected;
			selecting = SelectMode::mouse;
		}
	}
}

void FullscreenMenu::mouse_up(int mx, int my) {
	if (selecting != SelectMode::mouse)
		return;

	selecting = SelectMode::wait;

	const unsigned mselected = (unsigned)MenuButtonState::selected;
	const unsigned mactive = (unsigned)MenuButtonState::active;
	OrthogonalGroup &og = orthogonal[ogIndex];

	// special button that cannot be selected using keyboard navigation
	if (PointInMenuButton(&quitButton, mx, my) && !quitButton.is_hidden() && (quitButton.state & mactive))
		throw 0;

	quitButton.state &= ~mactive;

	if (!og.activated || og.selected >= og.buttonCount)
		return;

	MenuButton *btn = &og.buttons[og.selected];
	og.activated = false;

	if (PointInMenuButton(btn, mx, my) && !btn->is_hidden()) {
		btn->state = (btn->state | mselected) & ~mactive;
		og.fnActivate(og.selected);
	} else {
		btn->state &= ~mactive;
	}
}

void FullscreenMenu::key_tapped(GameKey key) {
	if (selecting != SelectMode::keyboard)
		return;

	OrthogonalGroup &og = orthogonal[ogIndex];

	if (key == GameKey::ui_select || key == GameKey::ui_back) {
		selecting = SelectMode::wait;
		unsigned idx = og.selected;
		og.buttons[idx].state &= ~(unsigned)MenuButtonState::active;

		if (key == GameKey::ui_back)
			idx = -1;

		og.fnActivate(idx);
	}
}

void FullscreenMenu::key_down(GameKey key, KeyboardController &keyctl, Audio &sfx) {
	if (selecting != SelectMode::wait)
		return;

	OrthogonalGroup &og = orthogonal[ogIndex];

	if (key == GameKey::ui_back || key == GameKey::ui_select) {
		selecting = SelectMode::keyboard;
		sfx.play_sfx(SfxId::ui_click);

		if (key == GameKey::ui_select)
			og.buttons[og.selected].state |= (unsigned)MenuButtonState::active;

		return;
	}

	unsigned old = og.selected;

	if (key == GameKey::ui_tabulate) {
		if (ogCount > 1) {
			tabulate();
			return;
		}

		inc(og.selected, og.buttonCount - 1);
	} else if (key == GameKey::ui_prev) {
		dec(og.selected);
	} else if (key == GameKey::ui_next) {
		inc_to(og.selected, og.buttonCount - 1);
	}

	if (og.selected != old) {
		unsigned mask = (unsigned)MenuButtonState::selected;

		og.buttons[old].state &= ~mask;
		og.buttons[og.selected].state |= mask;
	}
}

void FullscreenMenu::tabulate(unsigned steps) {
	unsigned index = (ogIndex + steps) % ogCount;

	// special case: reset
	if (steps == 0)
		index = 0;

	if (index == ogIndex)
		return;

	selecting = SelectMode::wait;

	OrthogonalGroup &old = orthogonal[ogIndex];
	OrthogonalGroup &next = orthogonal[ogIndex = index];

	old.unfocus();
	next.focus(next.selected);
}

void FullscreenMenu::drawButtons(Frame &f, BackgroundColors &col, Assets &ass, Audio &sfx) {
	for (unsigned i = 0; i < ogCount; ++i)
		orthogonal[i].show(f, col);

	quitButton.show(f, col);

	if (labelCount) {
		for (unsigned i = 0; i < labelCount; ++i)
			labels[i].show(f);
	}
}

}
}
