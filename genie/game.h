#ifndef GENIE_GAME_H
#define GENIE_GAME_H

#include "ui.h"
#include <SDL2/SDL.h>

struct genie_game {
	struct genie_ui *ui;
	int running;
};

extern struct genie_game genie_game;

void genie_game_init(struct genie_game*, struct genie_ui*);
int genie_game_main(struct genie_game*);

#endif
