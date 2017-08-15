/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_UI_H
#define GENIE_UI_H

#include "menu.h"
#include "shell.h"

#include <stddef.h>
#include <SDL2/SDL.h>

#define GENIE_UI_ERR_SUCCESS 0
#define GENIE_UI_ERR_SDL 1

#define GENIE_UI_MENU_STACKSZ 16
#define GENIE_UI_OPTIONSZ 16

struct genie_game;

struct genie_ui {
	const char *game_title;
	unsigned width, height;
	SDL_Window *win;
	SDL_GLContext *gl;

	struct genie_game *game;

	struct menu_nav *stack[GENIE_UI_MENU_STACKSZ];
	struct menu_nav *old_top;
	unsigned slots[GENIE_UI_OPTIONSZ];
	unsigned stack_index;
	int menu_press;

	int console_show;
	struct console console;
};

extern struct genie_ui genie_ui;

void genie_ui_free(struct genie_ui *ui);
int genie_ui_init(struct genie_ui *ui, struct genie_game *game);
int genie_ui_is_visible(const struct genie_ui *ui);

void genie_ui_display(struct genie_ui *ui);
void genie_ui_key_down(struct genie_ui *ui, SDL_Event *ev);
void genie_ui_key_up(struct genie_ui *ui, SDL_Event *ev);
void genie_ui_mouse_down(struct genie_ui *ui, SDL_Event *ev);
void genie_ui_mouse_up(struct genie_ui *ui, SDL_Event *ev);

void genie_ui_menu_update(struct genie_ui *ui);
void genie_ui_menu_push(struct genie_ui *ui, struct menu_nav *nav);
void genie_ui_menu_pop(struct genie_ui *ui);
struct menu_nav *genie_ui_menu_peek(const struct genie_ui *ui);

void genie_ui_hide(struct genie_ui *ui);
void genie_ui_show(struct genie_ui *ui);
void genie_ui_raise(struct genie_ui *ui);

#endif
