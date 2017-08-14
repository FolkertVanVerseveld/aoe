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
#define GENIE_MODE_NOVIDEO  2048

#define GENIE_VIDEO_MODE_NOINTRO   1
#define GENIE_VIDEO_MODE_NOLOGO    2
#define GENIE_VIDEO_MODE_GRPINTRO  4

#include <stddef.h>
#include "include/genie/engine.h"

extern unsigned genie_mode;
extern unsigned genie_video_mode;

int genie_open_help(void);

const char *genie_absolute_path(const char *path);
const char *genie_avi_path(char *str, size_t size, const char *path);

#endif
