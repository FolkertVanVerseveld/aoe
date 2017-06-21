#ifndef GENIE_UI_H
#define GENIE_UI_H

#include <SDL2/SDL.h>

#define GENIE_UI_ERR_SUCCESS 0
#define GENIE_UI_ERR_SDL 1

struct genie_ui {
	const char *game_title;
	unsigned width, height;
	SDL_Window *win;
	SDL_GLContext *gl;
};

extern struct genie_ui genie_ui;

void genie_ui_free(struct genie_ui*);
int genie_ui_init(struct genie_ui*);
int genie_ui_is_visible(const struct genie_ui*);
void genie_ui_display(struct genie_ui*);

#endif
