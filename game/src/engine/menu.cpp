#include "menu.hpp"
#include "../engine.hpp"

namespace aoe {

using namespace io;

const MenuInfo menu_info[] = {
	{ false, KeyboardMode::configure, DrsId::bkg_main_menu }, // init
	{ true, KeyboardMode::fullscreen_menu, DrsId::bkg_main_menu }, // start
	{ true, KeyboardMode::other, DrsId::bkg_singleplayer },
	{ true, KeyboardMode::other, DrsId::bkg_singleplayer },
	{ false, KeyboardMode::other, (DrsId)0 }, // sp game
	{ true, KeyboardMode::other, DrsId::bkg_multiplayer },
	{ true, KeyboardMode::other, DrsId::bkg_multiplayer },
	{ false, KeyboardMode::other, (DrsId)0 }, // mp game
	{ false, KeyboardMode::other, (DrsId)0 }, // mp settings
	{ true, KeyboardMode::other, DrsId::bkg_defeat },
	{ true, KeyboardMode::other, DrsId::bkg_victory },
	{ true, KeyboardMode::other, DrsId::bkg_editor_menu }, // edit menu
	{ false, KeyboardMode::other, (DrsId)0 }, // edit scn
};

const MenuInfo &GetMenuInfo(MenuState state) {
	unsigned idx = (unsigned)state, end = (unsigned)MenuState::max;
	return menu_info[idx < end ? idx : 0];
}

const MenuInfo *HasMenuInfo(MenuState state) {
	unsigned idx = (unsigned)state, end = (unsigned)MenuState::max;
	return idx < end ? &menu_info[idx] : NULL;
}

enum class MainMenuButtonIdx {
	singleplayer,
	multiplayer,
	help,
	scenario_builder,
	quit,
};

void MenuButtonActivate(MenuState state, unsigned idx)
{
	if (state != MenuState::start)
		return;

	MainMenuButtonIdx btn = (MainMenuButtonIdx)idx;

	switch (btn) {
	case MainMenuButtonIdx::singleplayer:
		next_menu_state = MenuState::singleplayer_menu;
		break;
	case MainMenuButtonIdx::multiplayer:
		next_menu_state = MenuState::multiplayer_menu;
		break;
	case MainMenuButtonIdx::help:
		eng->open_help();
		break;
	case MainMenuButtonIdx::scenario_builder:
		next_menu_state = MenuState::editor_menu;
		break;
	default:
		throw 0;
	}
}

}