#pragma once

#include <map>
#include <vector>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>

namespace aoe {

enum class GameKey {
	key_left,
	key_right,
	key_up,
	key_down,
	kill_entity,
	toggle_chat,
	toggle_pause,
	gamespeed_increase,
	gamespeed_decrease,
	toggle_fullscreen,
	open_help,
	toggle_debug_window,
	max,
};

class KeyboardController final {
	std::vector<bool> state, state_tapped;
	std::map<SDL_Keycode, GameKey> keys;
public:
	KeyboardController();

	void clear();

	GameKey down(const SDL_KeyboardEvent&);
	GameKey up(const SDL_KeyboardEvent&);

	bool is_down(GameKey);
	bool is_tapped(GameKey, bool reset=true);
};
}
