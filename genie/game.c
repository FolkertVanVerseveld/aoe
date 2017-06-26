/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "game.h"
#include "ui.h"

struct genie_game genie_game;

void genie_game_init(struct genie_game *g, struct genie_ui *ui)
{
	g->ui = ui;
}

int genie_game_main(struct genie_game *g)
{
	g->running = 1;

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
end:
	return 0;
}
