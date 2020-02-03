#pragma once

namespace genie {

enum class LangId {
	exit = 1002, /**< Quick exit button */
	btn_ok = 9858,
	btn_delete = 9417,
	error = 2001, /**< Generic error or unknown button text */
	/* Main menu stuff */
	title_main = 1001,
	btn_singleplayer = 9202,
	btn_multiplayer = 9203,
	btn_help = 9205,
	btn_edit = 9206,
	btn_exit = 9027,
	/* Main menu copyright stuff */
	main_copy1 = 9241,
	main_copy2a = 9243,
	main_copy2b = 9244,
	main_copy3 = 9253,
	title_singleplayer_menu = 9220,
	btn_random_map = 9226,
	btn_campaign = 9224,
	btn_deathmatch = 9227,
	btn_scenario = 9221,
	btn_savedgame = 9223,
	btn_cancel = 9225,
	/* Singleplayer random map settings */
	title_singleplayer = 9679,
	player_name = 9680,
	player_civilization = 9681,
	player_id = 9646,
	player_team = 9647,
	btn_start_game = 9472,
	btn_settings = 9682,
	player_list = 10340,
	player_1 = 10341,
	player_2 = 10342,
	player_3 = 10343,
	player_4 = 10344,
	player_5 = 10345,
	player_6 = 10346,
	player_7 = 10347,
	player_8 = 10348,
	player_you = 10349,
	player_computer = 9683,
};

}
