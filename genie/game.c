/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "game.h"
#include "ui.h"
#include "sfx.h"
#include <err.h>

#define GAME_INIT 1

#define STONE_AGE 0
#define TOOL_AGE 1
#define BRONZE_AGE 2
#define IRON_AGE 3

struct genie_game genie_game;

static void player_init(struct genie_player *p)
{
	genie_set_resources(&p->resources, 0, 0, 0, 0);
	p->pop = p->pop_cap = 0;
	p->state = 0;
	p->age = STONE_AGE;
	p->score.military = 0;
	p->score.economy = 0;
	p->score.technology = 0;
	p->score.alive = 0;
	p->score.wonder = 0;
}

void genie_set_resources(struct genie_resources *res, unsigned wood, unsigned food, unsigned gold, unsigned stone)
{
	res->wood = wood;
	res->food = food;
	res->gold = gold;
	res->stone = stone;
}

static void player_set_resources(struct genie_player *p, unsigned wood, unsigned food, unsigned gold, unsigned stone)
{
	genie_set_resources(&p->resources, wood, food, gold, stone);
}

struct genie_resources *genie_player_get_resources(const struct genie_game *g, unsigned i)
{
	return (struct genie_resources*)&g->stat.player[i].resources;
}

static void player_stat_init(struct genie_game_stat *this)
{
	unsigned n = 2;
	for (unsigned i = 0; i < n; ++i) {
		player_init(&this->player[i]);
		player_set_resources(&this->player[i], 200, 150, 0, 150);
	}
	this->players = n;
	this->pop_cap = 0;
}

void genie_game_init(struct genie_game *g, struct genie_ui *ui)
{
	g->ui = ui;
	g->init = 0;
	player_stat_init(&g->stat);
}

static int game_init(struct genie_game *g)
{
	int error = 1;

	if (g->init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}
	g->init = GAME_INIT;
	genie_msc_play(MSC_OPENING, 0);

	error = 0;
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
	genie_game_show(g);
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
			case SDL_MOUSEBUTTONDOWN:
				genie_ui_mouse_down(g->ui, &ev);
				break;
			case SDL_MOUSEBUTTONUP:
				genie_ui_mouse_up(g->ui, &ev);
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

void genie_game_hide(struct genie_game *this)
{
	genie_ui_hide(this->ui);
}

void genie_game_show(struct genie_game *this)
{
	genie_ui_show(this->ui);
}
