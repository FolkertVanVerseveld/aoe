#ifndef AOE_UI_FULLSCREENMENU_HPP
#define AOE_UI_FULLSCREENMENU_HPP 1

#include "../engine/keyctl.hpp"

struct ImGuiViewport;

namespace aoe {

class Assets;
class Audio;
class BackgroundColors;

namespace ui {

class Frame;

enum class MenuButtonState {
	active   = 0x01,
	disabled = 0x02,
	hovered  = 0x04,
	selected = 0x08,
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
	unsigned menuState;
	MenuButton *buttons;
	unsigned buttonCount;
	unsigned selected;
	void (*draw)(Frame&, Audio&, Assets&);

	FullscreenMenu(unsigned menuState, MenuButton *buttons, unsigned buttonCount,
		void (*fn)(Frame &, Audio &, Assets &));

	void reshape(ImGuiViewport *vp);

	void kbp_down(GameKey key);
	void mouse_down(int mx, int my);
};

}

extern ui::FullscreenMenu mainMenu;

}

#endif