/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef FS_H
#define FS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define FS_BUFSZ 4096

struct fs_blob {
	int fd;
	void *data;
	size_t size;
	unsigned mode;
};

void fs_blob_init(struct fs_blob *b);
void fs_blob_free(struct fs_blob *b);

#define FS_MODE_READ 0x01
#define FS_MODE_WRITE 0x02
#define FS_MODE_MAP 0x04

#define FS_ERR_OK 0
#define FS_ERR_NOENT 1
#define FS_ERR_NOMEM 2
#define FS_ERR_MAP 3
#define FS_ERR_UNKNOWN 4

int fs_blob_open(struct fs_blob *b, const char *path, unsigned mode);
void fs_blob_close(struct fs_blob *b);

#define FS_OPT_NEED_GAME 1
#define FS_OPT_NEED_CDROM 2

/**
 * Get path to game file. This is necessary to differentiate between
 * installation through wine or running directly from a CD-ROM/ISO.
 */
int fs_get_path(char *buf, size_t bufsz, const char *dir, const char *file, unsigned options);

void fs_game_path(char *buf, size_t bufsz, const char *file);
void fs_data_path(char *buf, size_t bufsz, const char *file);
void fs_help_path(char *buf, size_t bufsz, const char *file);
void fs_cdrom_path(char *buf, size_t bufsz, const char *file);
/**
 * Get path to CD-ROM audio tracks. This may fail, because it is a pain
 * in the ass to mount audio tracks properly on linux.
 */
int fs_cdrom_audio_path(char *buf, size_t bufsz, const char *file);

void fs_walk_campaign(void (*item)(void *arg, char *name), void *arg, char *buf, size_t bufsz);
int fs_walk_ext(const char *dir, const char *ext, void (*item)(void *arg, char *name), void *arg, char *buf, size_t bufsz, unsigned options);

#ifdef __cplusplus
}
#endif

#endif
