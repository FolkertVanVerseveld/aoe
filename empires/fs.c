/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "fs.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <libgen.h>

#include "../setup/dbg.h"
#include "../setup/def.h"

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
