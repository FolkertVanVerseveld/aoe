/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie Game Engine startup configuration
 *
 * Licensed under GNU's Affero General Public License v3.0
 * Copyright Folkert van Verseveld
 */

#ifndef GENIE_CFG_H
#define GENIE_CFG_H

#define GE_POP_MIN 25
#define GE_POP_MAX 200

#define GE_CFG_NO_INTRO          0x01
#define GE_CFG_NO_SOUND          0x02
#define GE_CFG_NO_MUSIC          0x04
#define GE_CFG_NO_AMBIENT        0x08
#define GE_CFG_USE_MIDI          0x10
#define GE_CFG_NORMAL_MOUSE      0x20
#define GE_CFG_GAME_HELP         0x40
/** Use whatever resolution the display has while in fullscreen mode. */
#define GE_CFG_NATIVE_RESOLUTION 0x80

// FIXME refactor to enum, add custom mode
#define GE_CFG_MODE_640x480  0
#define GE_CFG_MODE_800x600  1
#define GE_CFG_MODE_1024x768 2

extern struct ge_config {
	unsigned options;
	unsigned screen_mode;
	unsigned limit;
	float music_volume;
	float sound_volume;
	float scroll_speed;
} ge_cfg;

extern const struct ge_start_cfg {
	const char *title;
	/**
	 * A NULL terminated list of video cinematics, that will be played
	 * during startup. Any missing cinematics are skipped.
	 */
	const char **vfx;
	/**
	 * A NULL terminated list of in game background music. The first three
	 * items must be the main menu, victory and defeat music. All other
	 * items are played during gameplay. There must be at least four items.
	 */
	const char **msc;
} ge_start_cfg;

#endif
