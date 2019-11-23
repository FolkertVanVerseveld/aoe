/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef _GENIE_ENGINE_H
#define _GENIE_ENGINE_H

#include <genie/engine.h>

#include <SDL2/SDL.h>

/* internal game engine state. */
extern struct _ge_state {
	unsigned init;
	unsigned msc_count;
	SDL_Window *win;
	SDL_Renderer *ctx;
	Uint32 timer;
} _ge_state;

#endif
