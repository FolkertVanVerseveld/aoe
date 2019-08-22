/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Common used routines
 *
 * Licensed under Affero General Public License v3.0
 * Copyright Folkert van Verseveld
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
	#include <pwd.h>
#endif
#include <dirent.h>

#include "def.h"
#include "../empires/errno.h"

#define PANIC_BUFSZ 1024

char path_cdrom[PATH_MAX];
char path_wine[PATH_MAX];

int has_wine = 0;
int game_installed;

void *fmalloc(size_t size)
{
	void *ptr;
	if (!(ptr = malloc(size)))
		panic("Out of memory");
	return ptr;
}

void *frealloc(void *ptr, size_t size)
{
	void *blk;
	if (!(blk = realloc(ptr, size)))
		panic("Out of memory");
	return blk;
}

void show_error(const char *str)
{
	char buf[PANIC_BUFSZ];
	fprintf(stderr, "%s\n", str);
	// TODO escape str
	snprintf(buf, PANIC_BUFSZ, "zenity --error --text=\"%s\"", str);
	system(buf);
}

void show_error_format(const char *format, ...)
{
	char buf[PANIC_BUFSZ];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof buf, format, args);
	va_end(args);

	show_error(buf);
}

void show_error_code(int code)
{
	const char *str;

	switch (code) {
	case 0:
		str = "Well, this is interesting";
		break;
	case GENIE_ERR_NEED_WINE:
		str = "You need to have Wine installed for this to work";
		break;
	case GENIE_ERR_NO_GAME:
		str = "The original game is not installed or could not be found";
		break;
	default:
		if (code < GENIE_ERR_NEED_WINE)
			str = strerror(errno);
		else
			str = "Unknown error occurred";
		break;
	}

	show_error(str);
}

void panic(const char *str)
{
	fputs("fatal error occurred:\n", stderr);
	show_error(str);
	exit(1);
}

void panicf(const char *format, ...)
{
	char buf[1024];
	va_list args;

	va_start(args, format);

	vsnprintf(buf, sizeof buf, format, args);
	fputs("fatal error occurred:\n", stderr);
	show_error(buf);

	va_end(args);

	exit(1);
}

int find_wine_installation(void)
{
	char path[PATH_MAX];
	const char *user;
	struct passwd *pwd;
	int fd = -1;

	/*
	 * If we can find the system registry, assume wine is installed.
	 * If found, check if the game has already been installed.
	 */

#ifndef _WIN32
	pwd = getpwuid(getuid());
	user = pwd->pw_name;

	snprintf(path, PATH_MAX, "/home/%s/.wine/system.reg", user);
	if ((fd = open(path, O_RDONLY)) == -1)
		return 0;
	has_wine = 1;
	close(fd);

	snprintf(path, PATH_MAX, WINE_PATH_FORMAT "/Empires.exe", user);
	if ((fd = open(path, O_RDONLY)) == -1)
		return 0;
	close(fd);
	snprintf(path_wine, PATH_MAX, WINE_PATH_FORMAT, user);

	return 1;
#else
	const char *root = "C:/Program Files (x86)/Microsoft Games/Age of Empires";
	snprintf(path, PATH_MAX, "%s/Empires.exe", root);

	if ((fd = open(path, O_RDONLY)) == -1)
		return 0;
	close(fd);
	return 1;
#endif
}

int find_lib_lang(char *path)
{
	char buf[PATH_MAX];
	struct stat st;

	snprintf(buf, PATH_MAX, "%s/setupenu.dll", path);
	if (stat(buf, &st))
		return 0;

	strcpy(path_cdrom, path);
	return 1;
}

int find_setup_files(void)
{
	char path[PATH_MAX];
	const char *user;
	DIR *dir;
	struct dirent *item;
	struct passwd *pwd;
	int found = 0;

#ifndef _WIN32
	/*
	 * Try following paths in specified order:
	 * /media/cdrom
	 * /media/username/cdrom
	 * Finally, traverse every directory in /media/username
	 */

	if (find_lib_lang("/media/cdrom"))
		return 0;

	pwd = getpwuid(getuid());
	user = pwd->pw_name;
	snprintf(path, PATH_MAX, "/media/%s/cdrom", user);
	if (find_lib_lang(path))
		return 0;

	snprintf(path, PATH_MAX, "/media/%s", user);
	dir = opendir(path);
	if (!dir)
		return 0;

	errno = 0;
	while (item = readdir(dir)) {
		if (!strcmp(item->d_name, ".") || !strcmp(item->d_name, ".."))
			continue;

		snprintf(path, PATH_MAX, "/media/%s/%s", user, item->d_name);

		if (find_lib_lang(path)) {
			found = 1;
			break;
		}
	}

	closedir(dir);
#endif
	return found;

}
