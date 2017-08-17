/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_GAME_H
#define GENIE_GAME_H

#include "ui.h"
#include "include/genie/limits.h"

#include <SDL2/SDL.h>

#define GENIE_PLAYER_STATE_ALIVE 1
/* Whether controlled by A.I. */
#define GENIE_PLAYER_STATE_CPU 2

struct genie_game_player {
	unsigned wood, food, gold, stone;
	unsigned pop, pop_cap;
	unsigned state;
	struct {
		unsigned military;
		unsigned economy;
		unsigned technology;
		int alive;
		unsigned wonder;
	} score;
};

struct genie_game_stat {
	unsigned players;
	struct genie_game_player player[GENIE_MAX_PLAYERS];
	unsigned pop_cap;
};

struct genie_game {
	struct genie_ui *ui;
	int running;
	int init;
	struct genie_game_stat stat;
};

extern struct genie_game genie_game;

void genie_game_init(struct genie_game*, struct genie_ui*);
void genie_game_hide(struct genie_game*);
void genie_game_show(struct genie_game*);
int genie_game_main(struct genie_game*);

#endif
