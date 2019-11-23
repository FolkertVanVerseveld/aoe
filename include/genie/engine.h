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

/** Global engine state */
extern struct ge_state {
	/** The in-game music index that is being played that *doesn't* include
	 * the main, victory and defeat music. */
	unsigned music_index;
	/** The in-game music count (excluding main, victory, defeat!) */
	unsigned music_count;
} ge_state;

// maximum resolution supported by original game
#define MAX_BKG_WIDTH 1024
#define MAX_BKG_HEIGHT 768

void open_readme(void);
/**
 * Play the specified AVI cinematics. The engine first looks in the installed
 * directory and falls back to the CD-ROM path if it cannot be found. If the
 * video cannot be found at all, it acts like a NOP. Any errors while starting
 * or during playback are ignored.
 */
void ge_video_play(const char *name);

/**
 * Initialize the Genie Game Engine, parse any engine parameters and return
 * modified argc and argv where all engine parameters have been removed.
 */
int ge_init(int *pargc, char **argv);
/** Cleanup the Genie Game Engine. */
int ge_quit(void);

void ge_main(void);

#ifdef __cplusplus
}
#endif

#endif
