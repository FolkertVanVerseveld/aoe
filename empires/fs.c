/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "fs.h"

#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>

#include "../setup/def.h"

void fs_game_path(char *buf, size_t bufsz, const char *file)
{
	const char *user;
	struct passwd *pwd;

	pwd = getpwuid(getuid());
	user = pwd->pw_name;

	if (game_installed)
		snprintf(buf, bufsz, WINE_PATH_FORMAT "/%s", user, file);
	else
		snprintf(buf, bufsz, "%s/game/%s", path_cdrom, file);
}
