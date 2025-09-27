#pragma once

#include "../legacy/legacy.hpp"

namespace aoe {

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
	io::DrsId border_col;
};

extern const MenuInfo menu_info[(unsigned)MenuState::max];

const MenuInfo &GetMenuInfo(MenuState state);
const MenuInfo *HasMenuInfo(MenuState state);

}