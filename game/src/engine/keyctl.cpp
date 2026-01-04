#include "keyctl.hpp"

namespace aoe {

KeyboardController keyctl;

KeyboardController::KeyboardController() : state((size_t)GameKey::max, false), state_tapped(state.size(), false), keys() {
	// NOTE in theory, you can have multiple keys mapped to the same GameKey, but this can lead to unreliable results with key down state.
	keys[SDLK_UP] = GameKey::ui_prev;
	keys[SDLK_DOWN] = GameKey::ui_next;
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
	keys[' '] = GameKey::focus_entity;
}

void KeyboardController::clear() {
	state.clear();
	state_tapped.clear();
	size_t end = (size_t)GameKey::max;
	state.resize(end, false);
	state_tapped.resize(end, false);
}

GameKey KeyboardController::down(const SDL_KeyboardEvent &e) {
	auto it = keys.find(e.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	GameKey k = it->second;
	size_t idx = (size_t)k;

	state[idx] = true;
	state_tapped[idx] = false;
	return k;
}

GameKey KeyboardController::up(const SDL_KeyboardEvent &e) {
	auto it = keys.find(e.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	GameKey k = it->second;
	size_t idx = (size_t)k;

	state[idx] = false;
	state_tapped[idx] = true;
	return k;
}

bool KeyboardController::is_down(GameKey k) {
	return state.at((size_t)k);
}

bool KeyboardController::is_tapped(GameKey k, bool reset) {
	size_t idx = (size_t)k;
	bool v = state_tapped.at(idx);
	if (reset)
		state_tapped[idx] = false;
	return v;
}

}
