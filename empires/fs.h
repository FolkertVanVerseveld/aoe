/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef FS_H
#define FS_H

#include <stddef.h>

/*
 * Get path to game file. This is necessary to differentiate between
 * installation through wine or running directly from a CD-ROM/ISO.
 */
void fs_game_path(char *buf, size_t bufsz, const char *file);

#endif
