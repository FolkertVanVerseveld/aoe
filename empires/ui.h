/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef UI_H
#define UI_H

#include <SDL2/SDL_render.h>

#define WIDTH 800
#define HEIGHT 600

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(SDL_Renderer *renderer);
void ui_free(void);

void display(SDL_Renderer *renderer);

#ifdef __cplusplus
}
#endif

#endif
