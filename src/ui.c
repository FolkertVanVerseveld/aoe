#include "ui.h"
#include <string.h>

static void ui_menu_update(struct ui *ui)
{
	const struct menu_nav *nav = ui_menu_peek(ui);
	const struct menu_list *list = nav->list;
	unsigned n = list->count;
	for (unsigned i = 0; i < n; ++i)
		ui->option_width[i] = strlen(list->buttons[i]);
	ui->title_width = strlen(nav->title);
}

void ui_menu_push(struct ui *ui, struct menu_nav *nav)
{
	ui->stack[ui->stack_index++] = nav;
	ui_menu_update(ui);
}

void ui_menu_pop(struct ui *ui)
{
	if (!--ui->stack_index)
		return;
	ui_menu_update(ui);
}

struct menu_nav *ui_menu_peek(const struct ui *ui)
{
	unsigned i = ui->stack_index;
	return i ? ui->stack[i - 1] : NULL;
}
