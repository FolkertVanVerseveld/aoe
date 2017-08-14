/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine video playback subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "video.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "engine.h"
#include "system.h"

static int has_vlc = 0;
static int video_init = 0;

#define VLC_PATH "/usr/bin/cvlc"
#define VLC_OPTIONS "--fullscreen --play-and-exit --no-video-title-show"

static const char *video_start_list[] = {"logo1", "logo2", "intro", NULL};
static const char **video_start_nointro_list = video_start_list + 2;

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
		warnx("video: cannot find \"%s\": %s", name, strerror(errno));

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
	if (genie_mode & GENIE_MODE_NOVIDEO)
		return 0;

	retval = genie_video_path(path, sizeof path, name);
	if (retval)
		return retval;

	snprintf(cmd, sizeof cmd, "%s %s \"%s\"", VLC_PATH, VLC_OPTIONS, path);
	return genie_system(cmd);
}

static int genie_video_play_list_simple(const char **list)
{
	for (const char **ptr = list; *ptr; ++ptr)
		genie_video_play(*ptr);
	return 0;
}

static int genie_video_play_list(const char **list)
{
	int retval = 1;
	char cmd[4096];
	char path[4096];
	int n;
	unsigned pos = 0;

	video_lazy_init();
	if (!has_vlc)
		return 1;
	if (genie_mode & GENIE_MODE_NOVIDEO)
		return 0;

	n = snprintf(cmd, sizeof cmd, "%s %s", VLC_PATH, VLC_OPTIONS);
	if (n < 0)
		return 1;
	pos += (unsigned)n;

	for (const char **name = list; *name; ++name) {
		retval = genie_video_path(path, sizeof path, *name);
		if (retval)
			return retval;
		if (pos >= sizeof cmd)
			goto overflow;
		n = snprintf(cmd + pos, sizeof cmd - pos, " \"%s\"", path);
		if (n < 0) {
overflow:
			warnx("video: playlist does not fit in buffer");
			return genie_video_play_list_simple(list);
		}
		pos += (unsigned)n;
	}
	return genie_system(cmd);
}

int genie_play_intro(void)
{
	const char **list = video_start_list;

	if (genie_video_mode & GENIE_VIDEO_MODE_NOINTRO)
		return 0;

	if (genie_video_mode & GENIE_VIDEO_MODE_NOLOGO)
		list = video_start_nointro_list;

	if (genie_video_mode & GENIE_VIDEO_MODE_GRPINTRO)
		return genie_video_play_list(list);
	return genie_video_play_list_simple(list);
}
