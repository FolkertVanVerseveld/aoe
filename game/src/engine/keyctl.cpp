#include "keyctl.hpp"

#include <algorithm> // fill

namespace aoe {

KeyboardController keyctl;

/*
https://discourse.libsdl.org/t/scancode-vs-keycode/32860/5

the author of SDL says about key-/scancodes:

SDL is trying to support essentially 3 different types of keyboard input:

SDL scancodes are used for games that require position independent key input, e.g. an FPS that uses WASD for movement should use scancodes because the position is important (i.e. the key position shouldn't change on a French AZERTY keyboard, etc.)
SDL keycodes are used for games that use symbolic key input, e.g. `I' for inventory, `B' for bag, etc. This is useful for MMOs and other games where the label on the key is important for understanding what the action is. Typically shift or control modifiers on those keys do something related to the original function and aren't related to another letter that might be mapped there (e.g. Shift-B closes all bags, etc.)
SDL text input is used for games that have a text field for chat, or character names, etc.
Games don't always follow these usage patterns, but this is the way it was originally designed.
*/

enum class KeyType {
	scan, // position independent
	key , // symbolic key
	text, // input fields
};

KeyboardController::KeyboardController()
	: state((size_t)GameKey::max, false), state_tapped(state.size(), false)
	, scan_keys()
	, keys(), mode(KeyboardMode::fullscreen_menu)
{
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

	scan_keys[SDL_SCANCODE_UP] = GameKey::ui_prev;
	scan_keys[SDL_SCANCODE_DOWN] = GameKey::ui_next;
	scan_keys[SDL_SCANCODE_SPACE] = GameKey::ui_select;
	scan_keys[SDL_SCANCODE_ESCAPE] = GameKey::ui_back;
	scan_keys[SDL_SCANCODE_GRAVE] = GameKey::toggle_debug_window;
	scan_keys[SDL_SCANCODE_F1] = GameKey::open_help;
	scan_keys[SDL_SCANCODE_F11] = GameKey::toggle_fullscreen;
}

void KeyboardController::clear(KeyboardMode mode) {
	this->mode = mode;
	std::fill(state.begin(), state.end(), false);
	std::fill(state_tapped.begin(), state_tapped.end(), false);
}

GameKey KeyboardController::down(const SDL_KeyboardEvent &e) {
	if (mode == KeyboardMode::fullscreen_menu) {
		auto it = scan_keys.find(e.keysym.scancode);

		if (it == scan_keys.end())
			return GameKey::max;

		GameKey k = it->second;
		size_t idx = (size_t)k;

		if (state[idx])
			return GameKey::max; // no repeating keys

		state[idx] = true;
		state_tapped[idx] = false;
		return k;
	}

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
	if (mode == KeyboardMode::fullscreen_menu) {
		auto it = scan_keys.find(e.keysym.scancode);

		if (it == scan_keys.end())
			return GameKey::max;

		GameKey k = it->second;
		size_t idx = (size_t)k;

		if (!state[idx]) {
			state[idx] = false;
			return GameKey::max;
		}
		state_tapped[idx] = true;
		state[idx] = false;
		return k;
	}

	auto it = keys.find(e.keysym.sym);

	if (it == keys.end())
		return GameKey::max;

	GameKey k = it->second;
	size_t idx = (size_t)k;

	if (!state[idx]) {
		state[idx] = false;
		return GameKey::max;
	}
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
