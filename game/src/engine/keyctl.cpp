#include "keyctl.hpp"

namespace aoe {

KeyboardController::KeyboardController() : state((size_t)GameKey::max, false), state_tapped(state.size(), false), keys() {
	// NOTE in theory, you can have multiple keys mapped to the same GameKey, but this can lead to unreliable results with key down state.
	keys[SDLK_a] = GameKey::key_left;
	keys[SDLK_d] = GameKey::key_right;
	keys[SDLK_w] = GameKey::key_up;
	keys[SDLK_s] = GameKey::key_down;
	keys[SDLK_DELETE] = GameKey::kill_entity;
	keys[SDLK_RETURN] = GameKey::toggle_chat;
	keys[SDLK_RETURN2] = GameKey::toggle_chat;
}

void KeyboardController::clear() {
	state.clear();
	state.resize((size_t)GameKey::max, false);
}

GameKey KeyboardController::down(const SDL_KeyboardEvent &k) {
	auto it = keys.find(k.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	state[(size_t)it->second] = true;
	state_tapped[(size_t)it->second] = false;
	return it->second;
}

GameKey KeyboardController::up(const SDL_KeyboardEvent &k) {
	auto it = keys.find(k.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	state[(size_t)it->second] = false;
	state_tapped[(size_t)it->second] = true;
	return it->second;
}

bool KeyboardController::is_down(GameKey k) {
	return state.at((size_t)k);
}

bool KeyboardController::is_tapped(GameKey k, bool reset) {
	bool v = state_tapped.at((size_t)k);
	if (reset)
		state_tapped[(size_t)k] = false;
	return v;
}

}
