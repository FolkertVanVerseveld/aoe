/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_CFG_H
#define GENIE_CFG_H

#define POP_MIN 25
#define POP_MAX 200

#define CFG_NO_INTRO 0x01
#define CFG_NO_SOUND 0x02
#define CFG_NO_MUSIC 0x04
#define CFG_NO_AMBIENT 0x08
#define CFG_USE_MIDI 0x10
#define CFG_NORMAL_MOUSE 0x20
#define CFG_GAME_HELP 0x40

#define CFG_MODE_640x480 0
#define CFG_MODE_800x600 1
#define CFG_MODE_1024x768 2

extern struct config {
	unsigned options;
	unsigned screen_mode;
	unsigned limit;
	float music_volume;
	float sound_volume;
	float scroll_speed;
} cfg;

#endif
