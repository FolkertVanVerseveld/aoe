/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Audio subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 */

#ifndef SFX_H
#define SFX_H

#ifndef _WIN32
	#include <AL/al.h>
	#include <AL/alc.h>
#endif
#include <SDL2/SDL_mixer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SFX_CHANNEL_COUNT 20

#define SFX_BARRACKS 5022
#define SFX_TOWN_CENTER 5044
#define SFX_ACADEMY 5002

#define SFX_VILLAGER_MOVE 5006

#define SFX_VILLAGER1 5037
#define SFX_VILLAGER2 5053
#define SFX_VILLAGER3 5054
#define SFX_VILLAGER4 5158
#define SFX_VILLAGER5 5171

#define SFX_BUTTON4 50300
#define SFX_CHAT 50302
#define SFX_WIN 50320
#define SFX_LOST 50321
#define SFX_BUTTON_GAME 5036

#define MUS_MAIN 0
#define MUS_VICTORY 1
#define MUS_DEFEAT 2
#define MUS_GAME1 3
#define MUS_GAME2 4
#define MUS_GAME3 5
#define MUS_GAME4 6
#define MUS_GAME5 7
#define MUS_GAME6 8
#define MUS_GAME7 9
#define MUS_GAME8 10
#define MUS_GAME9 11
#define MUS_GAME10 12
/* alternative names */
#define MUS_CHOIRS 5

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
