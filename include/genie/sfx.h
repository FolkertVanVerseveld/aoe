/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Audio subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 */

#ifndef GENIE_SFX_H
#define GENIE_SFX_H

#include <SDL2/SDL_mixer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SFX_CHANNEL_COUNT 20

#define SFX_BUTTON4 50300
#define SFX_CHAT 50302
#define SFX_WIN 50320
#define SFX_LOST 50321
#define SFX_BUTTON_GAME 5036

#define MUS_MAIN 0
#define MUS_VICTORY 1
#define MUS_DEFEAT 2
#define MUS_GAME 3

void sfx_init(void);
void sfx_free(void);

void sfx_play(unsigned id);

void mus_play(unsigned id);

extern volatile int music_playing;
extern unsigned music_index;
extern int in_game;

#ifdef __cplusplus
}
#endif

#endif
