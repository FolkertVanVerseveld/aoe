/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "game.h"
#include "ui.h"
#include "sfx.h"
#include <err.h>

#define GAME_INIT 1

struct genie_game genie_game;

void genie_game_init(struct genie_game *g, struct genie_ui *ui)
{
	g->ui = ui;
	g->init = 0;
}

static int game_init(struct genie_game *g)
{
	int error = 1;

	if (g->init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}
	g->init = GAME_INIT;
	ge_msc_play(MSC_OPENING, 0);

	error = 0;
fail:
	return error;
}

static void game_free(struct genie_game *g)
{
	if (!g->init)
		return;

	g->init &= ~GAME_INIT;

	if (g->init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, g->init
		);
}

int genie_game_main(struct genie_game *g)
{
	int error = 1;

	g->running = 1;
	genie_ui_display(g->ui);

	error = game_init(g);
	if (error)
		goto end;

	while (g->running) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				goto end;
			case SDL_KEYDOWN:
				genie_ui_key_down(g->ui, &ev);
				break;
			case SDL_KEYUP:
				genie_ui_key_up(g->ui, &ev);
				break;
			}
		}

		genie_ui_display(g->ui);
	}
	error = 0;
end:
	game_free(g);
	return error;
}
