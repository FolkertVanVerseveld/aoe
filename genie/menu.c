/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "menu.h"

#include <stdlib.h>
#include <err.h>

#include "engine.h"
#include "game.h"
#include "prompt.h"
#include "ui.h"
#include "sfx.h"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x)[0])

static const struct genie_ui_button buttons_main[] = {
	{212, 222, 375, 50, "Single Player"},
	{212, 285, 375, 50, "Multiplayer"},
	{212, 347, 375, 50, "Help"},
	{212, 410, 375, 50, "Scenario Builder"},
	{212, 472, 375, 50, "Exit"},
}, buttons_single_player[] = {
	{212, 147, 375, 50, "Random Map"},
	{212, 210, 375, 50, "Campaign"},
	{212, 272, 375, 50, "Death Match"},
	{212, 335, 375, 50, "Scenario"},
	{212, 397, 375, 50, "Saved Game"},
	{212, 460, 375, 50, "Cancel"},
	{779, 4, 17, 17, "X"},
}, buttons_single_player_game[] = {
	{525, 62, 262, 37, "Settings"},
	{87, 550, 300, 37, "Start Game"},
	{412, 550, 300, 37, "Cancel"},
	{725, 550, 37, 37, "?"},
	{779, 4, 17, 17, "X"},
}, buttons_game[] = {
	{620, 0, 108, 19, "Diplomacy"},
	{728, 0, 72, 19, "Menu"},
	{765, 482, 30, 30, "S"},
	{765, 564, 30, 30, "?"},
}, buttons_game_menu[] = {
	{220, 113, 360, 30, "Quit Game"},
	{220, 163, 360, 30, "Achievements"},
	{220, 198, 360, 30, "Scenario Instructions"},
	{220, 233, 360, 30, "Save"},
	{220, 268, 360, 30, "Load"},
	{220, 303, 360, 30, "Restart"},
	{220, 338, 360, 30, "Game Settings"},
	{220, 373, 360, 30, "Help"},
	{220, 408, 360, 30, "About"},
	{220, 458, 360, 30, "Cancel"},
}, buttons_game_diplomacy[] = {
	{588, 425, 30, 30, "?"},
	{98, 465, 190, 30, "OK"},
	{303, 465, 190, 30, "Clear Tributes"},
	{508, 465, 190, 30, "Cancel"},
}, buttons_game_achievements[] = {
	{125, 551, 250, 37, "Timeline"},
	{425, 551, 250, 37, "Cancel"},
	{779, 4, 17, 17, "X"},
}, buttons_game_timeline[] = {
	{250, 551, 300, 37, "Back"},
	{779, 4, 17, 17, "X"},
}, buttons_multiplayer[] = {
	/* Name */
	/* Connection Type */
	{27, 260, 746, 23, "Internet TCP/IP Connection For DirectPlay"},
	{87, 550, 300, 37, "OK"},
	{412, 550, 300, 37, "Cancel"},
	{725, 550, 37, 37, "?"},
}, buttons_scenario_builder[] = {
	{212, 212, 375, 50, "Create Scenario"},
	{212, 275, 375, 50, "Edit Scenario"},
	{212, 337, 375, 50, "Campaign Editor"},
	{212, 400, 375, 50, "Cancel"},
};

const char *genie_ui_button_get_text(const struct genie_ui_button *this)
{
	return this->text;
}

static struct menu_list menu_list_main = {
	.buttons = buttons_main,
	.count = ARRAY_SIZE(buttons_main)
}, menu_list_single_player = {
	.buttons = buttons_single_player,
	.count = ARRAY_SIZE(buttons_single_player)
}, menu_list_single_player_game = {
	.buttons = buttons_single_player_game,
	.count = ARRAY_SIZE(buttons_single_player_game)
}, menu_list_game = {
	.buttons = buttons_game,
	.count = ARRAY_SIZE(buttons_game)
}, menu_list_game_menu = {
	.buttons = buttons_game_menu,
	.count = ARRAY_SIZE(buttons_game_menu)
}, menu_list_game_diplomacy = {
	.buttons = buttons_game_diplomacy,
	.count = ARRAY_SIZE(buttons_game_diplomacy)
}, menu_list_game_achievements = {
	.buttons = buttons_game_achievements,
	.count = ARRAY_SIZE(buttons_game_achievements)
}, menu_list_game_timeline = {
	.buttons = buttons_game_timeline,
	.count = ARRAY_SIZE(buttons_game_timeline)
}, menu_list_multiplayer = {
	.buttons = buttons_multiplayer,
	.count = ARRAY_SIZE(buttons_multiplayer)
}, menu_list_scenario_builder = {
	.buttons = buttons_scenario_builder,
	.count = ARRAY_SIZE(buttons_scenario_builder)
};

static void menu_nav_select_dummy(struct menu_nav *this)
{
	show_error(
		genie_ui_button_get_text(&this->list->buttons[this->index]),
		"Not implemented yet"
	);
}

static void menu_nav_select_main(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_single_player(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_single_player_game(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_game(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_game_menu(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_game_diplomacy(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_game_achievements(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_game_timeline(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_multiplayer(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_scenario_builder(struct genie_ui *ui, struct menu_nav *this);

struct menu_nav menu_nav_start = {
	.title = "???",
	.flags = 0,
	.index = 0,
	.list = &menu_list_main,
	.select = menu_nav_select_main,
	.display = genie_display_default,
};
static struct menu_nav menu_nav_single_player = {
	.title = "Single Player Menu",
	.flags = 0,
	.index = 0,
	.list = &menu_list_single_player,
	.select = menu_nav_select_single_player,
	.display = genie_display_default,
}, menu_nav_single_player_game = {
	.title = "Single Player Game",
	.flags = 0,
	.index = 1,
	.list = &menu_list_single_player_game,
	.select = menu_nav_select_single_player_game,
	.display = genie_display_single_player,
}, menu_nav_game = {
	.title = "game",
	.flags = 0,
	.index = 0,
	.list = &menu_list_game,
	.select = menu_nav_select_game,
	.display = genie_display_game,
}, menu_nav_game_menu = {
	.title = "Game Menu",
	.flags = 0,
	.index = 9,
	.list = &menu_list_game_menu,
	.select = menu_nav_select_game_menu,
	.display = genie_display_default,
}, menu_nav_game_diplomacy = {
	.title = "Diplomacy",
	.flags = 0,
	.index = 1,
	.list = &menu_list_game_diplomacy,
	.select = menu_nav_select_game_diplomacy,
	.display = genie_display_diplomacy,
}, menu_nav_game_achievements = {
	.title = "Achievements",
	.flags = 0,
	.index = 0,
	.list = &menu_list_game_achievements,
	.select = menu_nav_select_game_achievements,
	.display = genie_display_achievements,
}, menu_nav_game_timeline = {
	.title = "Achievements",
	.flags = 0,
	.index = 0,
	.list = &menu_list_game_timeline,
	.select = menu_nav_select_game_timeline,
	.display = genie_display_timeline,
}, menu_nav_multiplayer = {
	.title = "Multiplayer Connection",
	.flags = 0,
	.index = 0,
	.list = &menu_list_multiplayer,
	.select = &menu_nav_select_multiplayer,
	.display = genie_display_default,
}, menu_nav_scenario_builder = {
	"Scenario Builder",
	.flags = 0,
	.index = 0,
	.list = &menu_list_scenario_builder,
	.select = menu_nav_select_scenario_builder,
	.display = genie_display_default,
};

static void menu_nav_select_main(struct genie_ui *ui, struct menu_nav *n)
{
	switch (n->index) {
	case 0:
		genie_ui_menu_push(ui, &menu_nav_single_player);
		break;
	case 1:
		genie_ui_menu_push(ui, &menu_nav_multiplayer);
		break;
	case 2:
		if (genie_open_help())
			show_error("Help unavailable", "Could not find or open help");
		break;
	case 3:
		genie_ui_menu_push(ui, &menu_nav_scenario_builder);
		break;
	case 4:
		genie_ui_menu_pop(ui);
		break;
	default:
		menu_nav_select_dummy(n);
		break;
	}
}

static void menu_nav_select_single_player(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 0:
		genie_ui_menu_push(ui, &menu_nav_single_player_game);
		break;
	case 5:
		genie_ui_menu_pop(ui);
		break;
	case 6:
		exit(0);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_single_player_game(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 1:
		genie_ui_menu_pop2(ui, 1);
		genie_ui_menu_push(ui, &menu_nav_game);
		genie_msc_play(MSC_TRACK5, 0);
		break;
	case 2:
		genie_ui_menu_pop(ui);
		break;
	case 4:
		exit(0);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_game(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 0:
		genie_ui_menu_push(ui, &menu_nav_game_diplomacy);
		break;
	case 1:
		genie_ui_menu_push(ui, &menu_nav_game_menu);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_game_menu(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 0:
		genie_ui_menu_pop2(ui, 1);
		genie_ui_menu_push(ui, &menu_nav_game_achievements);
		genie_msc_play(MSC_DEFEAT, 0);
		break;
	case 1:
		genie_ui_menu_push(ui, &menu_nav_game_achievements);
		break;
	case 9:
		genie_ui_menu_pop(ui);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_game_diplomacy(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 3:
		genie_ui_menu_pop(ui);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_game_achievements(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 0:
		genie_ui_menu_push(ui, &menu_nav_game_timeline);
		break;
	case 1:
		genie_ui_menu_pop(ui);
		break;
	case 2:
		exit(0);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_game_timeline(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 0:
		genie_ui_menu_pop(ui);
		break;
	case 1:
		exit(0);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_multiplayer(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 2:
		genie_ui_menu_pop(ui);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_scenario_builder(struct genie_ui *ui, struct menu_nav *this)
{
	switch (this->index) {
	case 3:
		genie_ui_menu_pop(ui);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

void menu_nav_down(struct menu_nav *this, unsigned key)
{
	this->keys |= key;
	unsigned n = this->list->count;
	switch (key) {
	case MENU_KEY_DOWN:
		if (!(this->keys & MENU_KEY_SELECT))
			this->index = (this->index + 1) % n;
		break;
	case MENU_KEY_UP:
		if (!(this->keys & MENU_KEY_SELECT))
			this->index = (this->index + n - 1) % n;
		break;
	}
}

void menu_nav_up(struct menu_nav *this, unsigned key)
{
	this->keys &= ~key;
	if (key == MENU_KEY_SELECT)
		this->select(&genie_ui, this);
}
