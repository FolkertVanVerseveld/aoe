/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_ENGINE_H
#define GENIE_ENGINE_H

#define GENIE_MODE_NOSTART     1
#define GENIE_MODE_SYSMEM      2
#define GENIE_MODE_MIDI        4
#define GENIE_MODE_MSYNC       8
#define GENIE_MODE_NOSOUND    16
#define GENIE_MODE_MFILL      32
#define GENIE_MODE_640_480    64
#define GENIE_MODE_800_600   128
#define GENIE_MODE_1024_768  256
#define GENIE_MODE_NOMUSIC   512
#define GENIE_MODE_NMOUSE   1024

extern unsigned genie_mode;

#define GE_INIT_LEGACY_OPTIONS 1

int ge_init(int argc, char **argv, const char *title, unsigned options);
int ge_main(void);

#endif
