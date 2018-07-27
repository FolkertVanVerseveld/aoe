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
extern int running;

void ui_init(void);
void ui_free(void);

void display(void);

void keydown(SDL_KeyboardEvent *event);
void keyup(SDL_KeyboardEvent *event);

void mousedown(SDL_MouseButtonEvent *event);
void mouseup(SDL_MouseButtonEvent *event);

#ifdef __cplusplus
}
#endif

#endif
