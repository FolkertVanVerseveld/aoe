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

#include <stddef.h>

void genie_cdrom_free(void);
int genie_cdrom_init(const char *root_path);
const char *genie_cdrom_get_music_path(unsigned id);
const char *genie_cdrom_absolute_game_path(const char *path);
void genie_cdrom_path(char *str, size_t size, const char *file);
void genie_cdrom_path_format(char *str, size_t size, const char *format, const char *file);
void genie_cdrom_avi_path(char *str, size_t size, const char *file);
void genie_cdrom_ttf_path(char *str, size_t size, const char *file);

#endif
