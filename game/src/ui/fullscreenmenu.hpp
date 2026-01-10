#ifndef AOE_UI_FULLSCREENMENU_HPP
#define AOE_UI_FULLSCREENMENU_HPP 1

#include "../engine/keyctl.hpp"
#include "../engine/menu.hpp"

struct ImGuiViewport;

namespace aoe {

class Assets;
class Audio;
class BackgroundColors;

namespace ui {

class Frame;

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

enum class MenuButtonState {
	active   = 0x01, // pressed down, unique
	disabled = 0x02, // unselectable
	hovered  = 0x04, // mouse over element
	selected = 0x08, // activated, unique
};

class MenuButton final {
public:
	float relY;
	float x0, y0, x1, y1;
	const char *name, *tooltip;
	unsigned state;

	MenuButton(float y, const char *name, const char *tooltip=NULL, unsigned state=0)
		: relY(y), x0(0), y0(0), x1(0), y1(0)
		, name(name), tooltip(tooltip), state(state) {}

	void reshape(ImGuiViewport *vp);
	bool show(Frame &f, Audio &sfx, const BackgroundColors &col) const;
};

class FullscreenMenu final {
public:
	MenuState menuState;
	MenuButton *buttons;
	unsigned buttonCount;
	bool activated;
	unsigned selected;
	void (*draw)(Frame&, Audio&, Assets&);

	FullscreenMenu(MenuState menuState, MenuButton *buttons, unsigned buttonCount,
		void (*fn)(Frame &, Audio &, Assets &));

	void reshape(ImGuiViewport *vp);

	void key_down(GameKey key, KeyboardController &keyctl, Audio &sfx);
	void key_tapped(GameKey key);
	void mouse_down(int mx, int my, Audio &sfx);
	void mouse_up(int mx, int my);
};

}

void MenuButtonActivate(MenuState state, unsigned idx);

extern ui::FullscreenMenu mainMenu;

}

#endif