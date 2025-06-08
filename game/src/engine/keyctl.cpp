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
	keys[SDLK_F3] = GameKey::toggle_pause;
	keys[SDLK_KP_PLUS] = GameKey::gamespeed_increase;
	keys[SDLK_KP_MINUS] = GameKey::gamespeed_decrease;
	keys[SDLK_F11] = GameKey::toggle_fullscreen;
	keys[SDLK_F1] = GameKey::open_help;
	keys[SDLK_BACKQUOTE] = GameKey::toggle_debug_window;
	keys['b'] = keys['B'] = GameKey::open_buildmenu;
	keys['c'] = keys['C'] = GameKey::train_villager;
	keys['h'] = keys['H'] = GameKey::focus_towncenter;
	keys['t'] = keys['T'] = GameKey::train_melee1;
	keys['.'] = GameKey::focus_idle_villager;
}

void KeyboardController::clear() {
	state.clear();
	state_tapped.clear();
	state.resize((size_t)GameKey::max, false);
	state_tapped.resize((size_t)GameKey::max, false);
}

GameKey KeyboardController::down(const SDL_KeyboardEvent &e) {
	auto it = keys.find(e.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	GameKey k = it->second;

	state[(size_t)k] = true;
	state_tapped[(size_t)k] = false;
	return k;
}

GameKey KeyboardController::up(const SDL_KeyboardEvent &e) {
	auto it = keys.find(e.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	GameKey k = it->second;

	state[(size_t)k] = false;
	state_tapped[(size_t)k] = true;
	return k;
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
