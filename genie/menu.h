/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_MENU_H
#define GENIE_MENU_H

struct genie_ui;

/* NOTE internal use only */

struct genie_ui_button {
	int x, y;
	int w, h;
	const char *text;
};

const char *genie_ui_button_get_text(const struct genie_ui_button *btn);

struct menu_list {
	const struct genie_ui_button *buttons;
	unsigned count;
};

struct menu_nav {
	const char *title;
	unsigned flags;
	unsigned index;
	unsigned keys;
	struct menu_list *list;
	void (*select)(struct genie_ui *ui, struct menu_nav *nav);
	void (*display)(struct genie_ui *ui);
};

#define MENU_KEY_DOWN   1
#define MENU_KEY_UP     2
#define MENU_KEY_SELECT 4

extern struct menu_nav menu_nav_start;

void menu_nav_down(struct menu_nav *nav, unsigned key);
void menu_nav_up(struct menu_nav *nav, unsigned key);

#endif
