/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_GAME_H
#define GENIE_GAME_H

#include "ui.h"
#include <SDL2/SDL.h>

struct genie_game {
	struct genie_ui *ui;
	int running;
	int init;
};

extern struct genie_game genie_game;

void genie_game_init(struct genie_game*, struct genie_ui*);
int genie_game_main(struct genie_game*);

#endif
