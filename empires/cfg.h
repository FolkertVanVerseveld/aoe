/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef CFG_H
#define CFG_H

#define POP_MIN 25
#define POP_MAX 200

#define CFG_NO_VIDEO 1
#define CFG_NO_SOUND 2
#define CFG_NO_MUSIC 4
#define CFG_NO_AMBIENT 8
#define CFG_USE_MIDI 16
#define CFG_NORMAL_MOUSE 32

#define CFG_MODE_640x480 0
#define CFG_MODE_800x600 1
#define CFG_MODE_1024x768 2

extern struct config {
	unsigned options;
	unsigned screen_mode;
	unsigned limit;
} cfg;

#endif
