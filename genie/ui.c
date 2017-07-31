/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"
#include <err.h>
#include <assert.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "game.h"
#include "prompt.h"
#include "gfx.h"

struct genie_ui genie_ui = {
	.game_title = "???",
	.width = 1024, .height = 768,
};

#define UI_INIT 1
#define UI_INIT_SDL 2

static unsigned ui_init = 0;

void genie_ui_free(struct genie_ui *ui)
{
	if (!ui_init)
		return;
	ui_init &= ~UI_INIT;

	if (ui->gl) {
		SDL_GL_DeleteContext(ui->gl);
		ui->gl = NULL;
	}
	if (ui->win) {
		SDL_DestroyWindow(ui->win);
		ui->win = NULL;
	}

	SDL_Quit();
	ui_init &= ~UI_INIT_SDL;

	if (ui_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, ui_init
		);
}

static int genie_ui_sdl_init(struct genie_ui *ui)
{
	int error = 0;

	if (SDL_Init(SDL_INIT_VIDEO))
		goto sdl_error;

	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)) {
sdl_error:
		show_error("SDL failed", SDL_GetError());
		error = GENIE_UI_ERR_SDL;
		goto fail;
	}

	ui->win = SDL_CreateWindow("AoE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ui->width, ui->height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!ui->win) {
		show_error("Engine failed", "No window");
		goto fail;
	}

	ui->gl = SDL_GL_CreateContext(ui->win);
	if (!ui->gl) {
		show_error(
		"Engine failed",
		"The graphics card could not be initialized because OpenGL\n"
		"drivers could not be loaded. Make sure your graphics card\n"
		"supports OpenGL 1.1 or better."
		);
		goto fail;
	}

	error = 0;
fail:
	return error;
}

int genie_ui_init(struct genie_ui *ui, struct genie_game *game)
{
	int error = 0;

	if (ui_init)
		return 0;

	ui_init = UI_INIT;

	error = genie_ui_sdl_init(ui);
	if (error)
		goto fail;

	menu_nav_start.title = ui->game_title;
	genie_ui_menu_push(ui, &menu_nav_start);

	ui->menu_press = 0;
	ui->game = game;

	error = 0;
fail:
	return error;
}

int genie_ui_is_visible(const struct genie_ui *ui)
{
	return ui->win != NULL && ui->gl != NULL;
}

static void genie_ui_update(struct genie_ui *ui)
{
	SDL_GL_SwapWindow(ui->win);
}

static void draw_menu(struct genie_ui *ui)
{
	struct menu_nav *nav;
	const struct menu_list *list;
	unsigned i, n;

	nav = genie_ui_menu_peek(ui);

	if (!nav)
		return;

	list = nav->list;
	n = list->count;

	glColor3f(1, 1, 1);

	genie_gfx_draw_text(
		(ui->width - ui->title_width * GENIE_GLYPH_WIDTH) / 2.0,
		GENIE_GLYPH_HEIGHT,
		nav->title
	);

	for (i = 0; i < n; ++i) {
		if (i == nav->index)
			glColor3ub(255, 255, 0);
		else
			glColor3ub(237, 206, 186);

		genie_gfx_draw_text(
			(ui->width - ui->option_width[i] * GENIE_GLYPH_WIDTH) / 2.0,
			308 + 80 * i,
			list->buttons[i]
		);
	}
}

static void menu_key_down(struct genie_ui *ui, SDL_Event *ev)
{
	struct menu_nav *nav;

	nav = genie_ui_menu_peek(ui);

	if (!nav)
		return;

	switch (ev->key.keysym.sym) {
	case SDLK_DOWN:
		menu_nav_down(nav, MENU_KEY_DOWN);
		break;
	case SDLK_UP:
		menu_nav_down(nav, MENU_KEY_UP);
		break;
	case ' ':
		menu_nav_down(nav, MENU_KEY_SELECT);
		ui->menu_press = 1;
		break;
	}
}

static void menu_key_up(struct genie_ui *ui, SDL_Event *ev)
{
	struct menu_nav *nav;

	nav = genie_ui_menu_peek(ui);

	if (!nav)
		return;

	switch (ev->key.keysym.sym) {
	case ' ':
		menu_nav_up(nav, MENU_KEY_SELECT);
		ui->menu_press = 0;
		break;
	}
}

void genie_ui_key_down(struct genie_ui *ui, SDL_Event *ev)
{
	menu_key_down(ui, ev);
}

void genie_ui_key_up(struct genie_ui *ui, SDL_Event *ev)
{
	menu_key_up(ui, ev);
}

void genie_ui_display(struct genie_ui *ui)
{
	genie_gfx_clear_screen(0, 0, 0, 0);
	genie_gfx_setup_ortho(ui->width, ui->height);

	draw_menu(ui);

	genie_ui_update(ui);
}

void genie_ui_menu_update(struct genie_ui *ui)
{
	const struct menu_nav *nav;
	const struct menu_list *list;

	nav = genie_ui_menu_peek(ui);
	list = nav->list;

	for (unsigned i = 0, n = list->count; i < n; ++i)
		ui->option_width[i] = strlen(list->buttons[i]);

	ui->title_width = strlen(nav->title);
}

void genie_ui_menu_push(struct genie_ui *ui, struct menu_nav *nav)
{
	ui->stack[ui->stack_index++] = nav;

	genie_ui_menu_update(ui);
}

void genie_ui_menu_pop(struct genie_ui *ui)
{
	if (!--ui->stack_index) {
		ui->game->running = 0;
		return;
	}

	genie_ui_menu_update(ui);
}

struct menu_nav *genie_ui_menu_peek(const struct genie_ui *ui)
{
	unsigned i;

	i = ui->stack_index;

	return i ? ui->stack[i - 1] : NULL;
}
