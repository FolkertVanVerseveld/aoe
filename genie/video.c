/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine video playback subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "video.h"
#include "engine.h"
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

static int has_vlc = 0;
static int video_init = 0;

#define VLC_PATH "/usr/bin/cvlc"

static const char *video_start_list[] = {"logo1", "logo2", "intro", NULL};

static void try_find_vlc(void)
{
	struct stat st;
	has_vlc = stat(VLC_PATH, &st) == 0;
}

static void video_lazy_init(void)
{
	if (video_init)
		return;
	video_init = 1;
	try_find_vlc();
}

/* Try to find the specified avi file by searching for `name'.AVI and `name'.avi */
static int genie_video_path(char *str, size_t size, const char *name)
{
	int retval = 1;
	struct stat st;
	char path[4096];

	genie_avi_path(path, sizeof path, name);
	snprintf(str, size, "%s.AVI", path);
	if (!stat(str, &st))
		return 0;

	snprintf(str, size, "%s.avi", path);
	retval = stat(str, &st);
	if (retval)
		warnx("video: cannot find \"%s\": %s\n", name, strerror(errno));

	return retval;
}

int genie_video_play(const char *name)
{
	int retval = 1;
	char cmd[4096];
	char path[4096];

	video_lazy_init();
	if (!has_vlc)
		return 1;

	retval = genie_video_path(path, sizeof path, name);
	if (retval)
		return retval;

	snprintf(cmd, sizeof cmd, "%s --fullscreen --play-and-exit \"%s\"", VLC_PATH, path);
	puts(cmd);
	return system(cmd);
}

int genie_play_intro(void)
{
	for (const char **ptr = video_start_list; *ptr; ++ptr)
		genie_video_play(*ptr);
	return 0;
}
