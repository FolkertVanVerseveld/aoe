/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

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
	btn_exit = 9207,

	/* Main menu copyright stuff */
	main_copy1 = 9241,
	main_copy2a = 9243,
	main_copy2b = 9244,
	main_copy3 = 9253,

	/* Singleplayer menu */
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

	player_count = 9688,

	map_type_random = 9653,
	map_type_format = 9654,
	map_players = 9655,
	map_reveal_format = 9656,
	type_enabled = 9657,
	type_disabled = 9658,
	cheating_format = 9659,
	victory_format = 9660,
	victory_format_prercent = 9661,
	victory_format_time = 9662,
	status_ready = 9663,
	status_not_ready = 9664,

	/* Singleplayer random map scenario settings */
	title_singleplayer_secnario = 9750,

	players = 9727,

	/* Campaign select scenario */
	title_campaign_select_player = 9838,

	btn_campaign_new_player = 10752,
	btn_campaign_remove_player = 10753,
	campaign_player_name = 10755,

	title_campaign_new_player = 10756,

	/* Campaign select scenario */
	title_campaign_select_scenario = 11213,
	select_campaign = 11214,

	select_scenario = 9726,

	scenario_level = 11215,

	scenario_level_hardest = 11216,
	scenario_level_hard = 11217,
	scenario_level_moderate = 11218,
	scenario_level_easy = 11219,
	scenario_level_easiest = 11220,

	/* Game */
	btn_diplomacy = 9851,
	btn_menu = 10020,
	btn_score = 4131,

	paused = 9001,
	win = 9004,
	lost = 9005,

	/* Game menu */
	btn_game_quit = 9273,
	btn_game_achievements = 9271,
	btn_game_instructions = 9278,
	btn_game_save = 9272,
	btn_game_load = 9276,
	btn_game_restart = 9279,
	btn_game_settings = 9274,
	btn_game_about = 9209,
	btn_game_cancel = 9275,

	/* Game settings menu */
	title_game_settings = 9431,

	title_speed = 9439,
	btn_speed_normal = 9432,
	btn_speed_fast = 9433,
	btn_speed_hyper = 9434,

	title_music = 9435,

	title_sound = 9438,

	title_scroll = 9456,

	title_screen = 9439,
	btn_screen_small = 9447,
	btn_screen_normal = 9448,
	btn_screen_large = 9449,

	title_mouse = 9450,
	btn_mouse_normal = 9451,
	btn_mouse_one = 9452,

	title_help = 9453,
	btn_help_on = 9454,
	btn_help_off = 9455,

	title_path = 9741,

	btn_civtbl = 4400,

	/* Multiplayer menu */
	title_multiplayer_host = 9678,

	title_multiplayer = 9611,
	title_multiplayer_servers = 9631,
	multiplayer_name = 9612,
	multiplayer_type = 9613,
	multiplayer_gaming_zone = 9614,
	multiplayer_join = 9632,
	multiplayer_host = 9634,

	lobby_name = 9680,
	lobby_civ = 9681,
	lobby_settings = 9682,
	lobby_chat = 9687,

	lobby_ready = 9663,
	btn_lobby_start = 9710,

	/* Extended help and game settings menu */
	mode_640_480 = 9447,
	mode_800_600 = 9448,
	mode_1024_768 = 9449,

	/* Secnario builder menu */
	title_scenario_editor = 9261,
	btn_scenario_create = 9262,
	btn_scenario_edit = 9263,
	btn_scenario_campaign = 9265,
	btn_scenario_cancel = 9264,

	/* Scenario editor */
	btn_scenario_map = 10010,
	btn_scenario_terrain = 10011,
	btn_scenario_players = 10012,
	btn_scenario_units = 10013,
	btn_scenario_triggers = 10014,
	btn_scenario_messages = 10015,
	btn_scenario_video = 10016,
	btn_scenario_options = 10017,
	btn_scenario_diplomacy = 10018,
	btn_scenario_triggers_all = 10019,
	btn_scenario_menu = 10020,

	/* Scenario editor menu */
	btn_scenario_menu_save = 9281,
	btn_scenario_menu_save_as = 9282,
	btn_scenario_menu_quit = 9283,
	btn_scenario_menu_cancel = 9284,
	btn_scenario_menu_edit = 9285,
	btn_scenario_menu_test = 9286,

	/* Map type */
	map_type_islands = 10602,
	map_type_continents = 10603,
	map_type_normal = 10604,
	map_type_flat = 10605,
	map_type_bumpy = 10606,

	/* Map size */
	map_size_tiny = 10611,
	map_size_small = 10612,
	map_size_medium = 10613,
	map_size_large = 10614,
	map_size_huge = 10615,

	/* Ages */
	age_stone = 4201,
	age_tool = 4202,
	age_bronze = 4203,
	age_iron = 4204,
	age_postiron = 4205,
	age_nomad = 4206,

	/* Resources */
	res_food = 4301,
	res_wood = 4302,
	res_stone = 4303,
	res_gold = 4304,

	/* Achievements */
	title_achievements = 9886,
	btn_restart = 9887,
	btn_military = 9900,
	btn_economy = 9901,
	btn_religion = 9902,
	btn_technology = 9903,
	survival = 9942,
	wonder = 9943,
	total_score = 9909,

	title_summary = 9945,
	btn_timeline = 9944,
	btn_close = 9884,
	btn_back = 9946,

	/* Civilizations */
	civ_egyptian = 10231,
	civ_greek = 10232,
	civ_babylonian = 10233,
	civ_assyrian = 10234,
	civ_minoan = 10235,
	civ_hittite = 10236,
	civ_phoenician = 10237,
	civ_sumerian = 10238,
	civ_persian = 10239,
	civ_shang = 10240,
	civ_yamato = 10241,
	civ_choson = 10242,

	/* Units */
	unit_unknown = 1205,
	unit_villager = 5131,
	building_town_center = 5255,
	building_barracks = 5008,
	building_academy = 13011,

	/* Gaia */
	unit_berry_bush = 5101,
	unit_desert_tree1 = 5301,
	unit_desert_tree2 = 5302,
	unit_desert_tree3 = 5303,
	unit_desert_tree4 = 5304,

	unit_gold = 5134,
	unit_stone = 5139,
};

}
