#include "menu.hpp"
#include "../engine.hpp"

#include "../ui/fullscreenmenu.hpp"
#include "../ui.hpp"

namespace aoe {

using namespace io;

const MenuInfo menu_info[] = {
	{ false, KeyboardMode::configure, DrsId::bkg_main_menu }, // init
	{ true, KeyboardMode::fullscreen_menu, DrsId::bkg_main_menu, &mainMenu }, // start
	{ true, KeyboardMode::fullscreen_menu, DrsId::bkg_singleplayer, &singleplayerMenu }, // sp menu
	{ true, KeyboardMode::fullscreen_menu, DrsId::bkg_singleplayer, &singleplayerHostMenu }, // sp host
	{ false, KeyboardMode::other, (DrsId)0 }, // sp game
	{ true, KeyboardMode::other, DrsId::bkg_multiplayer },
	{ true, KeyboardMode::other, DrsId::bkg_multiplayer },
	{ false, KeyboardMode::other, (DrsId)0 }, // mp game
	{ false, KeyboardMode::other, (DrsId)0 }, // mp settings
	{ true, KeyboardMode::other, DrsId::bkg_defeat },
	{ true, KeyboardMode::other, DrsId::bkg_victory },
	{ true, KeyboardMode::fullscreen_menu, DrsId::bkg_editor_menu, &scenarioMenu }, // edit menu
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

enum class SingleplayerMenuButtonIdx {
	random_map,
	campaign,
	death_match,
	scenario,
	saved_game,
	cancel,
};

enum class SingleplayerHostButtonIdx {
	settings,
	//player_table,
	start_game,
	cancel,
	//tooltip, // XXX do we need this?
};

enum class SingleplayerHostPlayerButtonIdx {
	remove_player,
	add_player,
	randomize,
	cancel,
};

enum class EditorMenuButtonIdx {
	create_scenario,
	load_scenario,
	campaign_editor,
	cancel
};

void StartMenuButtonActivate(unsigned idx)
{
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

void SingleplayerMenuButtonActivate(unsigned idx)
{
	SingleplayerMenuButtonIdx btn = (SingleplayerMenuButtonIdx)idx;

	switch (btn) {
	case SingleplayerMenuButtonIdx::random_map:
	case SingleplayerMenuButtonIdx::death_match:
		next_menu_state = MenuState::singleplayer_host;
		break;
	default:
		next_menu_state = MenuState::start;
		break;
	}
}

void SingleplayerHostTeamButtonActivate(unsigned idx)
{
	if (idx >= sp_players.size()) {
		next_menu_state = MenuState::singleplayer_menu;
		return;
	}

	PlayerSetting &ps = sp_players.at(idx);
	ui::MenuButton &btn = singleplayerHostTeamButtons[idx];

	ps.team = (ps.team + 1) % max_legacy_players;
	btn.name = ps.team ? std::to_string(ps.team) : "-";
}

static void set_player_count(unsigned count)
{
	if (count < 1)
		count = 1;
	else if (count >= max_legacy_players)
		count = max_legacy_players - 1;

	sp_player_count = count + 1;
	sp_player_ui_count = count;

	ui::MenuLabel &lblPlayers = singleplayerHostLabels[4];
	lblPlayers.name = std::to_string(count);

	ui::MenuButton *teams = singleplayerHostTeamButtons;

	for (unsigned i = 0; i < max_legacy_players - first_player_idx; ++i)
		teams[i].hide(i >= count);
}

void SingleplayerHostPlayerButtonCustom(GameKey key)
{
	switch (key) {
	case GameKey::ui_inc_players:
		SingleplayerHostPlayerButtonActivate((unsigned)SingleplayerHostPlayerButtonIdx::add_player);
		break;
	case GameKey::ui_dec_players:
		SingleplayerHostPlayerButtonActivate((unsigned)SingleplayerHostPlayerButtonIdx::remove_player);
		break;
	}
}

void SingleplayerHostPlayerButtonActivate(unsigned idx)
{
	ui::MenuLabel &lblPlayers = singleplayerHostLabels[4];

	switch ((SingleplayerHostPlayerButtonIdx)idx) {
	case SingleplayerHostPlayerButtonIdx::remove_player:
		set_player_count(sp_player_ui_count - 1);
		break;
	case SingleplayerHostPlayerButtonIdx::add_player:
		set_player_count(sp_player_ui_count + 1);
		break;
	case SingleplayerHostPlayerButtonIdx::randomize:
		// TODO randomize
		break;
	default:
		next_menu_state = MenuState::singleplayer_menu;
		break;
	}
}

void SingleplayerHostButtonActivate(unsigned idx)
{
	SingleplayerHostButtonIdx btn = (SingleplayerHostButtonIdx)idx;

	switch (btn) {
	case SingleplayerHostButtonIdx::settings:
	case SingleplayerHostButtonIdx::start_game:
		break;
	default:
		next_menu_state = MenuState::singleplayer_menu;
		break;
	}
}

void EditorMenuButtonActivate(unsigned idx)
{
	EditorMenuButtonIdx btn = (EditorMenuButtonIdx)idx;

	switch (btn) {
	case EditorMenuButtonIdx::create_scenario:
		next_menu_state = MenuState::editor_scenario;
		break;
	default:
		next_menu_state = MenuState::start;
		break;
	}
}

}