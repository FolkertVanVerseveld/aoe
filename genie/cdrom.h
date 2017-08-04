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

void genie_cdrom_free(void);
int genie_cdrom_init(void);
const char *genie_cdrom_get_music_path(unsigned id);
const char *genie_cdrom_absolute_game_path(const char *path);

#endif
