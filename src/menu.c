#include "menu.h"
#include "memmap.h"
#include "todo.h"
#include "ui.h"
#include "../dbg.h"

static struct menu_ctl_vtbl menu_ctl_vtbl = {
	.dtor = menu_ctl_dtor,
	// TODO populate
}, menu_ctl_vtbl_0 = {
	.dtor = menu_ctl_dtor_stdiobuf,
	// TODO populate
};

struct menu_ctl *menu_ctl_dtor(struct menu_ctl *this, char a2)
{
	stub
	(void)this;
	(void)a2;
	return this;
}

struct menu_ctl *menu_ctl_dtor_stdiobuf(struct menu_ctl *this, char call_this_dtor)
{
	stub
	if (call_this_dtor & 1)
		delete(this);
	return this;
}

static struct menu_ctl *menu_ctl_vtbl_init_0(struct menu_ctl *this, const char *str)
{
	stub
	(void)str;
	halt();
	return this;
}

static struct menu_ctl *menu_ctl_vtbl_init_1(struct menu_ctl *this)
{
	stub
	halt();
	return this;
}

static struct menu_ctl *menu_ctl_vtbl_init(struct menu_ctl *this, const char *title)
{
	menu_ctl_vtbl_init_0(this, title);
	this->vtbl = &menu_ctl_vtbl_0;
	menu_ctl_vtbl_init_1(this);
	return this;
}

struct menu_ctl *menu_ctl_init_title(struct menu_ctl *this, const char *title)
{
	menu_ctl_vtbl_init(this, title);
	this->vtbl = &menu_ctl_vtbl;
	return this;
}

struct regpair *menu_gameC24(struct menu_ctl *this, struct regpair *a2, struct regpair *a3)
{
	stub
	(void)this;
	(void)a2;
	(void)a3;
	halt();
	return NULL;
}

struct game3F4 *ui_ctl(struct menu_ctl *this, int v4, int player_id)
{
	stub
	(void)this;
	(void)v4;
	(void)player_id;
	halt();
	return NULL;
}

static const char *buttons_main[] = {
	"Single Player",
	"Multiplayer",
	"Help (not implemented)",
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

struct menu_list menu_list_main = {
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
	fprintf(stderr, "%s: not implemented yet\n", this->list->buttons[this->index]);
}

static void menu_nav_select_main(struct menu_nav *this);
static void menu_nav_select_single_player(struct menu_nav *this);

struct menu_nav menu_nav_main = {
	.title = "Age of Empires",
	.flags = 0,
	.index = 0,
	.list = &menu_list_main,
	.select = menu_nav_select_main
}, menu_nav_single_player = {
	"Single Player Menu",
	.flags = 0,
	.index = 0,
	.list = &menu_list_single_player,
	.select = menu_nav_select_single_player
};

static void menu_nav_select_main(struct menu_nav *this)
{
	switch (this->index) {
	case 0:
		ui_menu_push(&ui, &menu_nav_single_player);
		break;
	case 4:
		ui_menu_pop(&ui);
		break;
	default:
		menu_nav_select_dummy(this);
		break;
	}
}

static void menu_nav_select_single_player(struct menu_nav *this)
{
	switch (this->index) {
	case 5:
		ui_menu_pop(&ui);
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
		this->select(this);
}
