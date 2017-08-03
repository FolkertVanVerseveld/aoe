/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine CD-ROM handling
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * This is an internal header and should not be included directly.
 */

#ifndef GENIE_CDROM_H
#define GENIE_CDROM_H

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

void ge_cdrom_free(void);
int ge_cdrom_init(void);
const char *ge_cdrom_get_music_path(unsigned id);
const char *ge_cdrom_absolute_path(const char *path);

#endif
