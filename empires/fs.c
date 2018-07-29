/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "fs.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "../setup/def.h"

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

void fs_help_path(char *buf, size_t bufsz, const char *file)
{
	snprintf(buf, bufsz, "%s/game/help/%s", path_cdrom, file);
}
