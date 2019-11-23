/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <genie/fs.h>
#include <genie/cfg.h>

#include <xt/os_macros.h>

// FIXME PATH_MAX simply isn't
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#if XT_IS_WINDOWS
#include <xt/string.h>
#endif

void ge_video_play(const char *name)
{
	if (ge_cfg.options & GE_CFG_NO_INTRO)
		return;
	char path[PATH_MAX], buf[PATH_MAX];

	fs_get_path(path, sizeof path, "avi/", name, 0);
	if (access(path, F_OK | R_OK)) {
		// retry and force to read from CD-ROM
		fs_get_path(path, sizeof path, "game/avi/", name, FS_OPT_NEED_CDROM);

		if (access(path, F_OK | R_OK)) {
			fprintf(stderr, "%s: file not found or readable\n", path);
			return;
		}
	}

#if XT_IS_WINDOWS
	/*
	 * system is executed as CMD /C, which means that in case the specified
	 * command starts with ", the last " is removed and this would break
	 * our command. this is addressed by enclosing the whole command...
	 * also, vlc does not accept / as directory separator...
	 */
	xtStringReplaceAll(path, '/', '\\');
	snprintf(buf, sizeof buf, "\"\"C:\\Program Files (x86)\\VideoLAN\\VLC\\vlc.exe\" --play-and-exit --fullscreen \"%s\"\"", path);

	int code = system(buf);
	if (code < 0 || code > 0xff)
		fputs("video playback error\n", stderr);
#else
	/*
	 * we have no real way to check if ffplay is installed, so we just try
	 * it and see if it fails. an unknown command error yields code 0x7f00
	 * on my machine...
	 *
	 * if ffplay fails for whatever reason, try cvlc and don't bother
	 * checking if that worked because there are no real distro independent
	 * alternatives to try after that point...
	 */
	snprintf(buf, sizeof buf, "ffplay -fs -loop 1 -autoexit \"%s\"", path);
	int code = system(buf);
	if (code < 0 || code == 0x7f00) {
		// probably command not found... try cvlc
		snprintf(buf, sizeof buf, "cvlc --play-and-exit -f \"%s\"", path);
		code = system(buf);
		if (code < 0 || code > 0xff)
			fputs("video playback error\n", stderr);
	}
#endif
}
