#include "menu.h"

#include <err.h>

#include "prompt.h"
#include "ui.h"

static const char *buttons_main[] = {
	"Single Player",
	"Multiplayer",
	"Help",
	"Scenario Builder",
	"Exit"
}, *buttons_single_player[] = {
	"Random Map",
	"Campaign",
	"Death Match",
	"Scenario",
	"Saved Game",
	"Cancel"
}, *buttons_multiplayer[] = {
	"Name",
	"Connection Type",
	"OK",
	"Cancel"
}, *buttons_scenario_builder[] = {
	"Create Scenario",
	"Edit Scenario",
	"Campaign Editor",
	"Cancel"
};

static struct menu_list menu_list_main = {
	.buttons = buttons_main,
	.count = 5
}, menu_list_single_player = {
	.buttons = buttons_single_player,
	.count = 6
}, menu_list_multiplayer = {
	.buttons = buttons_multiplayer,
	.count = 4
}, menu_list_scenario_builder = {
	.buttons = buttons_scenario_builder,
	.count = 4
};

static void menu_nav_select_dummy(struct menu_nav *this)
{
	show_error(
		this->list->buttons[this->index],
		"Not implemented yet"
	);
}

static void menu_nav_select_main(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_single_player(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_multiplayer(struct genie_ui *ui, struct menu_nav *this);
static void menu_nav_select_scenario_builder(struct genie_ui *ui, struct menu_nav *this);

struct menu_nav menu_nav_start = {
	.title = "???",
	.flags = 0,
	.index = 0,
	.list = &menu_list_main,
	.select = menu_nav_select_main
};
static struct menu_nav menu_nav_single_player = {
	.title = "Single Player Menu",
	.flags = 0,
	.index = 0,
	.list = &menu_list_single_player,
	.select = menu_nav_select_single_player
}, menu_nav_multiplayer = {
	.title = "Multiplayer Connection",
	.flags = 0,
	.index = 0,
	.list = &menu_list_multiplayer,
	.select = &menu_nav_select_multiplayer
}, menu_nav_scenario_builder = {
	"Scenario Builder",
	.flags = 0,
	.index = 0,
	.list = &menu_list_scenario_builder,
	.select = menu_nav_select_scenario_builder
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
	case 5:
		genie_ui_menu_pop(ui);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_multiplayer(struct genie_ui *ui, struct menu_nav *this)
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
	if (key == MENU_KEY_SELECT) {
		//sfx_play(SFX_BTN1, 0, 1.0f);
		this->select(&genie_ui, this);
	}
}
