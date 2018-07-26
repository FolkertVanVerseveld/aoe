/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef UI_H
#define UI_H

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>

#define WIDTH 800
#define HEIGHT 600

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

extern SDL_Renderer *renderer;

void ui_init(void);
void ui_free(void);

bool display(void);

bool keydown(SDL_KeyboardEvent *event);
bool keyup(SDL_KeyboardEvent *event);

#ifdef __cplusplus
}
#endif

#endif
