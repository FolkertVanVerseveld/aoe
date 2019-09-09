/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200809L
#endif

#include <genie/fs.h>

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
	#include <sys/mman.h>
#else
	#include <windows.h>
	#include <io.h>
#endif
#include <unistd.h>
#include <fcntl.h>
#ifndef _WIN32
	#include <pwd.h>
#endif
#include <dirent.h>
#include <libgen.h>

#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/error.h>
#include <genie/genie.h>

#ifdef _WIN32
	#define INVALID_MAPPING NULL
#else
	#define INVALID_MAPPING MAP_FAILED
#endif

// MAP_FILE may be undefined because it is legacy
#ifndef MAP_FILE
	#define MAP_FILE 0
#endif

void fs_blob_init(struct fs_blob *b)
{
	b->fd = -1;
	b->data = INVALID_MAPPING;
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
#ifndef _WIN32
		unsigned flags = mode & FS_MODE_READ ? PROT_READ : PROT_READ | PROT_WRITE;
		unsigned mflags = MAP_FILE | (mode & FS_MODE_READ ? MAP_SHARED : MAP_PRIVATE);

		if ((b->data = mmap(NULL, st.st_size, flags, mflags, fd, 0)) == MAP_FILE) {
			close(fd);
			return FS_ERR_MAP;
		}
#else
		HANDLE fm, h;
		DWORD protect, desiredAccess;
		DWORD dwMaxSizeLow, dwMaxSizeHigh;

		dwMaxSizeLow = sizeof(off_t) <= sizeof(DWORD) ? (DWORD)st.st_size : (DWORD)(st.st_size & 0xFFFFFFFFL);
		dwMaxSizeHigh = sizeof(off_t) <= sizeof(DWORD) ? (DWORD)0 : (DWORD)((st.st_size >> 32) & 0xFFFFFFFFL);

		if (mode & FS_MODE_WRITE) {
			protect = PAGE_READWRITE;
			desiredAccess = FILE_MAP_READ | FILE_MAP_WRITE;
		} else {
			protect = PAGE_READONLY;
			desiredAccess = FILE_MAP_READ;
		}

		if ((h = (HANDLE)_get_osfhandle(fd)) == INVALID_HANDLE_VALUE || !(fm = CreateFileMapping(h, NULL, protect, dwMaxSizeHigh, dwMaxSizeLow, NULL))) {
			if (h != INVALID_HANDLE_VALUE)
				CloseHandle(h);
			close(fd);
			return FS_ERR_MAP;
		}

		if (!(b->data = MapViewOfFile(fm, desiredAccess, 0, 0, st.st_size))) {
			CloseHandle(fm);
			CloseHandle(h);
			close(fd);
			return FS_ERR_UNKNOWN;
		}

		// store the handles somewhere
		//CloseHandle(fm);
		//CloseHandle(h);
#endif
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
#ifndef _WIN32
		if (b->size)
			munmap(b->data, b->size);
#else
		if (!UnmapViewOfFile(b->data))
			fprintf(stderr, "Cannot unmap address %p\n", b->data);
#endif
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

#ifndef _WIN32
static const char *username()
{
	return getpwuid(getuid())->pw_name;
}
#else
static char fs_username[256];

static const char *username()
{
	DWORD count = sizeof fs_username;
	GetUserNameA(fs_username, &count);
	return fs_username;
}
#endif

int fs_get_path(char *buf, size_t bufsz, const char *dir, const char *file, unsigned options)
{
	if (options & FS_OPT_NEED_CDROM)
		return snprintf(buf, bufsz, "%s/%s%s", path_cdrom, dir, file);

	if (game_installed || (options & FS_OPT_NEED_GAME))
#ifndef _WIN32
		return snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s%s", username(), dir, file);
#else
		return snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s%s", dir, file);
#endif

	return snprintf(buf, bufsz, game_installed ? "%s/%s%s" : "%s/game/%s%s", path_cdrom, dir, file);
}

void fs_game_path(char *buf, size_t bufsz, const char *file)
{
	if (game_installed)
#ifndef _WIN32
		snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s", username(), file);
#else
		snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s", file);
#endif
	else
		snprintf(buf, bufsz, "%s/game/%s", path_cdrom, file);

	printf("game path: %s\n", buf);
}

void fs_data_path(char *buf, size_t bufsz, const char *file)
{
	fs_get_path(buf, bufsz, "data/", file, 0);
	if (!game_installed)
		strtolower(buf + strlen(path_cdrom) + 1);
}

// TODO merge with fs_walk_ext
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
#ifndef _WIN32
				snprintf(buf, bufsz, WINE_PATH_FORMAT "/campaign/%s", username(), entry->d_name);
#else
				snprintf(buf, bufsz, WINE_PATH_FORMAT "/campaign/%s", entry->d_name);
#endif
			else
				snprintf(buf, bufsz, "%s/game/campaign/%s", path_cdrom, entry->d_name);
			item(arg, buf);
		}
	}

	if (d)
		closedir(d);
}

int fs_walk_ext(const char *dir, const char *accept, void (*item)(void *arg, char *name), void *arg, char *buf, size_t bufsz, unsigned options)
{
	DIR *d = NULL;
	struct dirent *entry;

	if (!game_installed && (options & FS_OPT_NEED_GAME))
		return GENIE_ERR_NO_GAME;

	fs_get_path(buf, bufsz, dir, "", options);

	if (!(d = opendir(buf))) {
		fprintf(stderr, "cannot read %s: %s\n", dir, strerror(errno));
		fprintf(stderr, "path: %s\n", buf);
		return errno;
	}

	while ((entry = readdir(d))) {
		char *ext;

		if ((ext = strrchr(entry->d_name, '.')) && !strcasecmp(ext + 1, accept)) {
#ifndef _WIN32
			snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s/%s", username(), dir, entry->d_name);
#else
			snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s/%s", dir, entry->d_name);
#endif
			item(arg, buf);
		}
	}

	if (d)
		closedir(d);

	return 0;
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
#ifndef _WIN32
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
#else
	fs_get_path(buf, bufsz, "sound/", file, 0);
	return access(buf, F_OK | R_OK);
#endif
}

void open_readme(void)
{
	char path[FS_BUFSZ];
	fs_cdrom_path(path, sizeof path, "readme.doc");
#ifndef _WIN32
	char buf[FS_BUFSZ];
	snprintf(buf, sizeof buf, "xdg-open \"%s\"", path);
	system(buf);
#else
	ShellExecute(GetDesktopWindow(), "open", path, NULL, NULL, SW_SHOWNORMAL);
#endif
}
