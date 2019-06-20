/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "fs.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>
#include <libgen.h>

#include "../setup/dbg.h"
#include "../setup/def.h"

void fs_blob_init(struct fs_blob *b)
{
	b->fd = -1;
	b->data = MAP_FAILED;
	b->size = 0;
}

void fs_blob_free(struct fs_blob *b)
{
	if (b->fd != -1)
		fs_blob_close(b);
	#if DEBUG
	fs_blob_init(b);
	#endif
}

int fs_blob_open(struct fs_blob *b, const char *path, unsigned mode)
{
	int fd;
	struct stat st;

	if ((fd = open(path, mode & FS_MODE_READ ? O_RDONLY : O_RDWR)) == -1
		|| fstat(fd, &st))
	{
		if (errno == ENOENT)
			return FS_ERR_NOENT;
		return FS_ERR_UNKNOWN;
	}

	if (mode & FS_MODE_MAP) {
		unsigned flags = mode & FS_MODE_READ ? PROT_READ : PROT_READ | PROT_WRITE;
		unsigned mflags = MAP_FILE | (mode & FS_MODE_READ ? MAP_SHARED : MAP_PRIVATE);

		if ((b->data = mmap(NULL, st.st_size, flags, mflags, fd, 0)) == MAP_FILE) {
			close(fd);
			return FS_ERR_MAP;
		}
	} else {
		if (!(b->data = malloc(st.st_size))) {
			close(fd);
			return FS_ERR_NOMEM;
		}
		if (read(fd, b->data, st.st_size) != st.st_size) {
			close(fd);
			return FS_ERR_MAP;
		}
	}

	b->fd = fd;
	b->size = st.st_size;
	b->mode = mode;
	return FS_ERR_OK;
}

void fs_blob_close(struct fs_blob *b)
{
	if (b->mode & FS_MODE_MAP) {
		if (b->size)
			munmap(b->data, b->size);
	} else {
		free(b->data);
	}
	if (b->fd != -1)
		close(b->fd);
}

static void strtolower(char *str)
{
	for (unsigned char *ptr = (unsigned char*)str; *ptr; ++ptr)
		*ptr = tolower(*ptr);
}

static const char *username()
{
	return getpwuid(getuid())->pw_name;
}

void fs_game_path(char *buf, size_t bufsz, const char *file)
{
	if (game_installed)
		snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s", username(), file);
	else
		snprintf(buf, bufsz, "%s/game/%s", path_cdrom, file);
}

void fs_data_path(char *buf, size_t bufsz, const char *file)
{
	if (game_installed)
		snprintf(buf, bufsz, WINE_PATH_FORMAT "/data/%s", username(), file);
	else {
		snprintf(buf, bufsz, "%s/game/data/%s", path_cdrom, file);
		strtolower(buf + strlen(path_cdrom) + 1);
	}
}

void fs_walk_campaign(void (*item)(void *arg, char *name), void *arg, char *buf, size_t bufsz)
{
	DIR *d = NULL;
	struct dirent *entry;

	fs_game_path(buf, bufsz, "campaign/");

	if (!(d = opendir(buf))) {
		perror("cannot read campaigns");
		fprintf(stderr, "path: %s\n", buf);
		return;
	}

	while ((entry = readdir(d))) {
		char *ext;

		if ((ext = strrchr(entry->d_name, '.')) && !strcmp(ext + 1, "cpn")) {
			if (game_installed)
				snprintf(buf, bufsz, WINE_PATH_FORMAT "/game/campaign/%s", username(), entry->d_name);
			else
				snprintf(buf, bufsz, "%s/game/campaign/%s", path_cdrom, entry->d_name);
			item(arg, buf);
		}
	}

	if (d)
		closedir(d);
}

void fs_help_path(char *buf, size_t bufsz, const char *file)
{
	snprintf(buf, bufsz, "%s/game/help/%s", path_cdrom, file);
}

void fs_cdrom_path(char *buf, size_t bufsz, const char *file)
{
	snprintf(buf, bufsz, "%s/%s", path_cdrom, file);
}

#define BLKID_TYPE "TYPE=\""

int fs_cdrom_audio_path(char *buf, size_t bufsz, const char *file)
{
	/*
	 * Main strategy:
	 * * Find all mounted iso9660 devices using blkid
	 * * Check if file exists for each mounted iso9660 device
	 */
	char line[1024];
	char devname[64];
	int err = 1;

	FILE *f = popen("blkid", "r");

	if (!f) {
		perror("blkid");
		return 1;
	}

	while (fgets(line, sizeof line, f)) {
		if (sscanf(line, "%63s:", devname) != 1 || !*devname)
			goto end;

		devname[strlen(devname) - 1] = '\0';

		char *devtype, *end;
		if ((devtype = strstr(line, BLKID_TYPE)) && (end = strchr(devtype + strlen(BLKID_TYPE), '"'))) {
			*end = '\0';
			devtype += strlen(BLKID_TYPE);
			//dbgf("device: %s, type: %s\n", basename(devname), devtype);
			if (!strcmp(devtype, "iso9660")) {
				struct stat st;
				snprintf(buf, bufsz, "/run/user/1000/gvfs/cdda:host=%s/%s", basename(devname), file);
				if (stat(buf, &st) == 0) {
					err = 0;
					goto end;
				}
			}
		}
	}

end:
	pclose(f);
	return err;
}
