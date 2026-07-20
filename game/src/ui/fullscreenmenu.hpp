#ifndef AOE_UI_FULLSCREENMENU_HPP
#define AOE_UI_FULLSCREENMENU_HPP 1

#include "../engine/keyctl.hpp"
#include "../engine/menu.hpp"
#include "align.hpp"

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
	hidden   = 0x10, // invisible and uninteractable
};

enum class MenuButtonLayoutType {
	vertical, // main, singleplayer menu, scenario menu
	corner,   // "X", "?" buttons
	relative, // gameplay_settings singleplayer settings
	//custom,   // TODO multiplayer menu, etc.
};

struct MenuButtonLayoutRelative final {
	int relX, relY; // relative position
	int w, h;
};

struct MenuButtonLayoutCorner final {
	bool topRight; // when false: bottomRight
	int margin; // to corner
	int w, h;
};

union MenuButtonLayoutData final {
	float relY; // vertical
	MenuButtonLayoutCorner corner;
	MenuButtonLayoutRelative rel;
	//SDL_Rect custom;

	MenuButtonLayoutData(float y) : relY(y) {}
	MenuButtonLayoutData(const MenuButtonLayoutCorner &c) : corner(c) {}
	MenuButtonLayoutData(const MenuButtonLayoutRelative &r) : rel(r) {}
	//MenuButtonLayoutData(const SDL_Rect &bnds) : custom(bnds) {}
};

class MenuLabel final {
public:
	SDL_FRect bnds;
	std::string name;
	MenuButtonLayoutRelative layout;
	TextHalign halign;

	MenuLabel(const MenuButtonLayoutRelative &layout, const char *name, TextHalign halign);

	void reshape(ImGuiViewport *vp);
	void show(Frame &f) const;
};

class MenuButton final {
public:
	SDL_FRect bnds;
	std::string name;
	const char *tooltip;
	unsigned state;
	MenuButtonLayoutType type;
	MenuButtonLayoutData layout;

	MenuButton(float y, const char *name, const char *tooltip=NULL, unsigned state=0);
	MenuButton(const MenuButtonLayoutCorner &c, const char *name, unsigned state=0);
	MenuButton(MenuButtonLayoutType type, const MenuButtonLayoutData &layout, const char *name, unsigned state=0);

	void reshape(ImGuiViewport *vp);
	bool show(Frame &f, const BackgroundColors &col) const;

	void hide(bool v) {
		if (v)
			state |= (unsigned)MenuButtonState::hidden;
		else
			state &= ~(unsigned)MenuButtonState::hidden;
	}

	bool is_hidden() const noexcept {
		return state & (unsigned)MenuButtonState::hidden;
	}
};

class OrthogonalGroup final {
public:
	MenuButton *buttons;
	unsigned buttonCount;
	bool activated;
	unsigned selected;
	void (*fnActivate)(unsigned);

	OrthogonalGroup(MenuButton *buttons, unsigned buttonCount, void (*fnActivate)(unsigned), unsigned selected=0);

	void reshape(ImGuiViewport *vp);
	void show(Frame &f, BackgroundColors &col);

	void unfocus();
	void focus(unsigned index);
};

// TODO add custom group

// type of menu selection that's happening right now
enum class SelectMode {
	wait,
	keyboard,
	mouse,
};

class FullscreenMenu final {
public:
	MenuState menuState;
	OrthogonalGroup *orthogonal;
	unsigned ogIndex, ogCount;
	MenuLabel *labels;
	unsigned labelCount;
	SelectMode selecting; // TODO consider moving this to Engine
	const char *frameTitle; // for Frame
	const char *title;

	FullscreenMenu(MenuState menuState, OrthogonalGroup &orthogonal, const char *frameTitle, const char *title, MenuLabel *labels=NULL, unsigned labelCount=0);
	FullscreenMenu(MenuState menuState, OrthogonalGroup *orthogonal, unsigned ogCount, const char *frameTitle, const char *title, MenuLabel *labels=NULL, unsigned labelCount=0);

	void reshape(ImGuiViewport *vp);

	void key_down(GameKey key, KeyboardController &keyctl, Audio &sfx);
	void key_tapped(GameKey key);
	void mouse_down(int mx, int my, Audio &sfx);
	void mouse_up(int mx, int my);

	void tabulate(unsigned steps=1);

	void drawButtons(Frame &f, BackgroundColors &col, Assets &ass, Audio &sfx);
};

// special button that is always present in fullscreen menus
// when selected: close application immediately
extern MenuButton quitButton;

}

void StartMenuButtonActivate(unsigned idx);
void SingleplayerMenuButtonActivate(unsigned idx);
void EditorMenuButtonActivate(unsigned idx);

void SingleplayerHostButtonActivate(unsigned idx);
void SingleplayerHostTeamButtonActivate(unsigned idx);
void SingleplayerHostPlayerButtonActivate(unsigned idx);

extern ui::MenuButton singleplayerHostTeamButtons[8];
extern ui::MenuLabel singleplayerHostLabels[5];

extern ui::FullscreenMenu mainMenu, singleplayerMenu, scenarioMenu, singleplayerHostMenu;

}

#endif