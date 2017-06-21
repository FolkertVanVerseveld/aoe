#include "game.h"
#include "ui.h"

struct genie_game genie_game;

void genie_game_init(struct genie_game *g, struct genie_ui *ui)
{
	g->ui = ui;
}

int genie_game_main(struct genie_game *g)
{
	while (1) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				goto end;
			}
		}

		genie_ui_display(g->ui);
	}
end:
	return 0;
}
