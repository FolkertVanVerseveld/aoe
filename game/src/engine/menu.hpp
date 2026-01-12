#pragma once

#include "../legacy/legacy.hpp"
#include "keyboard_mode.hpp"

namespace aoe {

namespace ui {

class FullscreenMenu;

}

enum class MenuState {
	init,
	start,
	singleplayer_menu,
	singleplayer_host,
	singleplayer_game,
	multiplayer_menu,
	multiplayer_host,
	multiplayer_settings,
	multiplayer_game,
	defeat,
	victory,
	editor_menu,
	editor_scenario,
	max,
};

struct MenuInfo final {
	bool draw_border;
	KeyboardMode keyboard_mode;
	io::DrsId border_col;
	ui::FullscreenMenu *menu;
};

extern const MenuInfo menu_info[(unsigned)MenuState::max];

const MenuInfo &GetMenuInfo(MenuState state);
const MenuInfo *HasMenuInfo(MenuState state);

}