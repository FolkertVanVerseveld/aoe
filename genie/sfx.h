/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine Audio subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * This is an internal header and should not be included directly.
 * Unfortunately, SDL2_mixer does not support different audio formats played
 * together and applying effects is not nearly as complete as with OpenAL.
 *
 * So, for the music tracks we will use SDL2_mixer because we don't need any
 * effects except fade-in/fade-out and OpenAL for the sound effects.
 */

#ifndef GENIE_SFX_H
#define GENIE_SFX_H

#include "cdrom.h"

#define MSC_INVALID 0
#define MSC_OPENING 1
#define MSC_VICTORY 2
#define MSC_DEFEAT 3

#define MSC_TRACK5 4
#define MSC_TRACK6 5
#define MSC_TRACK7 6
#define MSC_TRACK8 7
#define MSC_TRACK9 8
#define MSC_TRACK10 9
#define MSC_TRACK11 10
#define MSC_TRACK12 11
#define MSC_TRACK13 12
#define MSC_TRACK14 13
/* alternative names */
#define MSC_CHOIRS 5

#define SFX_MENU_BUTTON 50300

void ge_sfx_free(void);
int ge_sfx_init(void);

int ge_msc_play(unsigned id, int loops);
void ge_msc_stop(void);
int ge_sfx_play(unsigned id);

#endif
