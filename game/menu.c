/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "menu.h"
#include <genie/dbg.h>
#include <genie/memmap.h>
#include <genie/todo.h>
#include "ui.h"
#include "sfx.h"

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
