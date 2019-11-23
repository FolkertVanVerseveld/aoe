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
/**
 * Play the specified AVI cinematics. The engine first looks in the installed
 * directory and falls back to the CD-ROM path if it cannot be found. If the
 * video cannot be found at all, it acts like a NOP.
 *
 * We have no real way to verify that any video playback application is
 * installed, so we just try to call it and ignore if it fails.
 */
void ge_video_play(const char *name);

/**
 * Initialize the Genie Game Engine, parse any engine parameters and return
 * modified argc and argv where all engine parameters have been removed. The
 * last argument is a NULL terminated list of video cinematics, that will be
 * played during startup. Any missing cinematics are skipped.
 */
int ge_init(int *pargc, char **argv, const char **vfx);
/** Cleanup the Genie Game Engine. */
int ge_quit(void);

#ifdef __cplusplus
}
#endif

#endif
