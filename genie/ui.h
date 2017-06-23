#ifndef GENIE_UI_H
#define GENIE_UI_H

#include "menu.h"

#include <stddef.h>
#include <SDL2/SDL.h>

#define GENIE_UI_ERR_SUCCESS 0
#define GENIE_UI_ERR_SDL 1

#define GENIE_UI_MENU_STACKSZ 16
#define GENIE_UI_OPTION_WIDTHSZ 64

struct genie_game;

struct genie_ui {
	const char *game_title;
	unsigned width, height;
	SDL_Window *win;
	SDL_GLContext *gl;

	struct genie_game *game;

	struct menu_nav *stack[GENIE_UI_MENU_STACKSZ];
	unsigned stack_index;
	size_t option_width[GENIE_UI_OPTION_WIDTHSZ];
	size_t title_width;
	int menu_press;
};

extern struct genie_ui genie_ui;

void genie_ui_free(struct genie_ui *ui);
int genie_ui_init(struct genie_ui *ui, struct genie_game *game);
int genie_ui_is_visible(const struct genie_ui *ui);

void genie_ui_display(struct genie_ui *ui);
void genie_ui_key_down(struct genie_ui *ui, SDL_Event *ev);
void genie_ui_key_up(struct genie_ui *ui, SDL_Event *ev);

void genie_ui_menu_update(struct genie_ui *ui);
void genie_ui_menu_push(struct genie_ui *ui, struct menu_nav *nav);
void genie_ui_menu_pop(struct genie_ui *ui);
struct menu_nav *genie_ui_menu_peek(const struct genie_ui *ui);

#endif
