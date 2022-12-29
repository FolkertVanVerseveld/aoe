#pragma once

#include <map>
#include <vector>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>

namespace aoe {

// TODO extract as keyboard controller
enum class GameKey {
	key_left,
	key_right,
	key_up,
	key_down,
	max,
};

class KeyboardController final {
	std::vector<bool> state;
	std::map<SDL_Keycode, GameKey> keys;
public:
	KeyboardController();

	void clear();

	GameKey down(const SDL_KeyboardEvent&);
	GameKey up(const SDL_KeyboardEvent&);

	bool is_down(GameKey);
};
}
