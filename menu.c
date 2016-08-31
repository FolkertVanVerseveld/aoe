#include "menu.h"
#include "todo.h"
#include "dbg.h"

static struct menu_ctl_vtbl menu_ctl_vtbl = {
	// TODO populate
}, menu_ctl_vtbl_0 = {
	// TODO populate
};

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
