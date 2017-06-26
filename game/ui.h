/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef AOE_UI_H
#define AOE_UI_H

#include "menu.h"
#include <stddef.h>

#define UI_MENU_STACKSZ 16
#define UI_OPTION_WIDTHSZ 64

struct ui {
	struct menu_nav *stack[UI_MENU_STACKSZ];
	unsigned stack_index;
	size_t option_width[UI_OPTION_WIDTHSZ];
	size_t title_width;
} ui;

void ui_menu_push(struct ui *ui, struct menu_nav *nav);
void ui_menu_pop(struct ui *ui);
struct menu_nav *ui_menu_peek(const struct ui *ui);

#endif
