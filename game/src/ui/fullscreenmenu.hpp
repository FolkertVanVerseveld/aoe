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

enum class MenuButtonState {
	active   = 0x01, // pressed down, unique
	disabled = 0x02, // unselectable
	hovered  = 0x04, // mouse over element
	selected = 0x08, // activated, unique
};

enum class MenuButtonLayoutType {
	vertical, // main, singleplayer menu, scenario menu
	corner,   // "X", "?" buttons
	//custom,   // TODO multiplayer menu, etc.
};

struct MenuButtonLayoutCorner final {
	bool topRight; // when false: bottomRight
	int margin; // to corner
	int w, h;
};

union MenuButtonLayoutData final {
	float relY; // vertical
	MenuButtonLayoutCorner corner;
	//SDL_Rect custom;

	MenuButtonLayoutData(float y) : relY(y) {}
	MenuButtonLayoutData(const MenuButtonLayoutCorner &c) : corner(c) {}
	//MenuButtonLayoutData(const SDL_Rect &bnds) : custom(bnds) {}
};

class MenuButton final {
public:
	SDL_FRect bnds;
	const char *name, *tooltip;
	unsigned state;
	MenuButtonLayoutType type;
	MenuButtonLayoutData layout;

	MenuButton(float y, const char *name, const char *tooltip=NULL, unsigned state=0);
	MenuButton(const MenuButtonLayoutCorner &c, const char *name);

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
	const char *frameTitle; // for Frame
	const char *title;
	void (*fnActivate)(unsigned);

	FullscreenMenu(MenuState menuState, MenuButton *buttons, unsigned buttonCount,
		const char *frameTitle, const char *title, void (*fnActivate)(unsigned));

	void reshape(ImGuiViewport *vp);

	void key_down(GameKey key, KeyboardController &keyctl, Audio &sfx);
	void key_tapped(GameKey key);
	void mouse_down(int mx, int my, Audio &sfx);
	void mouse_up(int mx, int my);
};

// special button that is always present in fullscreen menus
// when selected: close application immediately
extern MenuButton quitButton;

}

void StartMenuButtonActivate(unsigned idx);
void SingleplayerMenuButtonActivate(unsigned idx);
void EditorMenuButtonActivate(unsigned idx);

extern ui::FullscreenMenu mainMenu, singleplayerMenu, scenarioMenu;

}

#endif