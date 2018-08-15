/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef FS_H
#define FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define FS_BUFSZ 4096

/**
 * Get path to game file. This is necessary to differentiate between
 * installation through wine or running directly from a CD-ROM/ISO.
 */
void fs_game_path(char *buf, size_t bufsz, const char *file);
void fs_data_path(char *buf, size_t bufsz, const char *file);
void fs_help_path(char *buf, size_t bufsz, const char *file);
void fs_cdrom_path(char *buf, size_t bufsz, const char *file);
/**
 * Get path to CD-ROM audio tracks. This may fail, because it is a pain
 * in the ass to mount audio tracks properly on linux.
 */
int fs_cdrom_audio_path(char *buf, size_t bufsz, const char *file);

#ifdef __cplusplus
}
#endif

#endif
