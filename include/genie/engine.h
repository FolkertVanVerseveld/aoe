/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie Game Engine.
 *
 * Licensed under GNU's Affero General Public License v3.0
 * Copyright Folkert van Verseveld
 *
 * This software is open source and free for private, non-commercial
 * use and for academic research.
 */

#ifndef GENIE_ENGINE_H
#define GENIE_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cfg.h"

// maximum resolution supported by original game
#define MAX_BKG_WIDTH 1024
#define MAX_BKG_HEIGHT 768

void open_readme(void);
void video_play(const char *name);

/**
 * Initialize the Genie Game Engine, parse any engine parameters and
 * return modified argc and argv where all engine parameters have been
 * removed.
 */
int GE_Init(int *pargc, char **argv);
/** Cleanup the Genie Game Engine. */
int GE_Quit(void);

#ifdef __cplusplus
}
#endif

#endif
