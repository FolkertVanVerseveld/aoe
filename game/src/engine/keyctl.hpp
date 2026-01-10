#ifndef AOE_KEYCTL_HPP
#define AOE_KEYCTL_HPP 1

#include <map>
#include <set>
#include <vector>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>

#include "keyboard_mode.hpp"

namespace aoe {

enum class GameKey {
	ui_prev,
	ui_next,
	ui_select,
	// gameplay
	key_left,
	key_right,
	key_up,
	key_down,
	kill_entity,
	focus_entity,
	focus_idle_villager,
	// general hud stuff
	toggle_chat,
	toggle_pause,
	gamespeed_increase,
	gamespeed_decrease,
	// engine general stuff
	toggle_fullscreen,
	open_help,
	toggle_debug_window,
	// hotkeys for logistics
	focus_towncenter,
	open_buildmenu,
	train_villager,
	train_melee1,
	max,
};

class KeyboardController final {
	std::vector<bool> state, state_tapped;
	std::map<SDL_Scancode, GameKey> scan_keys;
	std::map<SDL_Keycode, GameKey> keys;
	KeyboardMode mode;
public:
	KeyboardController();

	void clear(KeyboardMode mode);

	GameKey down(const SDL_KeyboardEvent&);
	GameKey up(const SDL_KeyboardEvent&);

	bool is_down(GameKey);
	bool is_tapped(GameKey, bool reset=true);
};

extern KeyboardController keyctl;

}

#endif