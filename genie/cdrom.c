/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "cdrom.h"
#include "engine.h"
#include "prompt.h"
#include "string.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <err.h>

#define GVFS_PATH "/run/user/1000/gvfs/"

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define CDROM_INIT 1

static int cdrom_init = 0;

/** all cdrom devices paths accepted if found in mount(8) */
static const char *mount_cdrom_paths[] = {
	"/dev/cdrom",
	"/dev/sr",
	NULL
};

static const char *cdrom_fsck_paths[] = {
	"aoesetup.exe",
	"eula.txt",
	"setupenu.dll",
	"game/empires.exe",
	"game/language.dll",
	NULL
};

static const char *cdrom_tracks_paths[] = {
	"Track 2.wav",
	"Track 3.wav",
	"Track 4.wav",
	"Track 5.wav",
	"Track 6.wav",
	"Track 7.wav",
	"Track 8.wav",
	"Track 9.wav",
	"Track 10.wav",
	"Track 11.wav",
	"Track 12.wav",
	"Track 13.wav",
	"Track 14.wav",
	NULL
};

#define CDROM_TRACKS ARRAY_SIZE(cdrom_tracks_paths)

static char mount_cdrom[4096];
static const char *cdrom_dev = NULL;
static const char *cdrom_dir = NULL;
static const char *cdrom_type = NULL;
static const char *cdrom_options = NULL;
static const char *cdrom_label = NULL;
static char cdrom_gvfs_dir[4096];
static char cdrom_msc_path[4096];
static char cdrom_abs_path[4096];

static int audio_on_default_dir = 0;
static int audio_on_cdrom = 0;

void ge_cdrom_free(void)
{
	if (!cdrom_init)
		return;

	cdrom_init &= ~CDROM_INIT;

	if (cdrom_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, cdrom_init
		);
}

static int is_cdrom_dev(const char *str)
{
	const char **ptr;

	for (ptr = mount_cdrom_paths; *ptr; ++ptr)
		if (strsta(str, *ptr))
			return 1;

	return 0;
}

static int set_cdrom_root(char *mount_str)
{
	char *ptr;
	const char *dev, *dir, *type, *options, *label = NULL;

	strncpy0(mount_cdrom, mount_str, sizeof mount_cdrom);

	for (ptr = mount_cdrom; *ptr && !isspace(*ptr);)
		++ptr;
	if (!ptr)
		return 1;
	if (!strsta(ptr, " on "))
		return 1;
	*ptr = '\0';
	dev = mount_cdrom;

	ptr += strlen(" on ");
	dir = ptr;

	while (*ptr && !isspace(*ptr))
		++ptr;
	if (!ptr)
		return 1;
	if (!strsta(ptr, " type "))
		return 1;
	*ptr = '\0';

	ptr += strlen(" type ");
	type = ptr;

	while (*ptr && !isspace(*ptr))
		++ptr;
	if (!strsta(ptr, " ("))
		return 1;
	*ptr = '\0';

	ptr += strlen(" (");
	options = ptr;

	if (!(ptr = strchr(ptr, ')')))
		return 1;
	*ptr++ = '\0';

	if ((ptr = strchr(ptr, '[')) != NULL) {
		char *end;

		puts(ptr);

		end = strrchr(ptr, ']');
		if (!end)
			return 1;

		*end = '\0';
		label = ptr + 1;
	}

	printf("dev: %s\ndir: %s\ntype: %s\noptions: %s\nlabel: %s\n", dev, dir, type, options, label ? label : "");

	cdrom_dev = dev;
	cdrom_dir = dir;
	cdrom_type = type;
	cdrom_options = options;
	cdrom_label = label;

	return 0;
}

/** very simple file system check to make sure we have the necessary files */
static int cdrom_fsck(void)
{
	char path[4096];
	const char **ptr;
	struct stat st;

	for (ptr = cdrom_fsck_paths; *ptr; ++ptr) {
		snprintf(path, sizeof path, "%s/%s", cdrom_dir, *ptr);
		if (stat(path, &st)) {
			perror(path);
			return 1;
		}
	}

	return 0;
}

#define ERR_CDROM_SUCCESS 0
#define ERR_CDROM_MOUNT 1
#define ERR_CDROM_NODRIVE 2
#define ERR_CDROM_FSCK 3
#define ERR_CDROM_UNKNOWN 4

static int cdrom_is_mounted(void)
{
	char buf[4096];
	FILE *p;
	size_t n;
	int error = ERR_CDROM_NODRIVE;

	p = popen("/bin/mount -l", "r");
	if (!p) {
		perror("mount");
		error = ERR_CDROM_MOUNT;
		goto fail;
	}

	while (fgets(buf, sizeof buf, p)) {
		n = strlen(buf);
		if (n)
			buf[n - 1] = '\0';
		if (is_cdrom_dev(buf)) {
			set_cdrom_root(buf);

			if (cdrom_fsck()) {
				error = ERR_CDROM_FSCK;
				goto fail;
			} else
				goto end;
		}
	}
	if (!feof(p)) {
		perror("mount");
		error = ERR_CDROM_MOUNT;
		goto fail;
	}

	goto fail;
end:
	error = ERR_CDROM_SUCCESS;
fail:
	if (p)
		pclose(p);

	return error;
}

/* try to find audio on CD-ROM by searching the found path and try /run/user/1000/gvfs if it fails */
static void cdrom_try_load_audio(void)
{
	char path[4096];
	char dev[1024];
	const char *host;
	struct stat st;

	if (!cdrom_dev || !cdrom_dir)
		return;
	if (genie_mode & GENIE_MODE_NOMUSIC)
		return;
	/* try cdrom_dir first */
	audio_on_default_dir = 1;
	for (const char **ptr = cdrom_tracks_paths; *ptr; ++ptr) {
		snprintf(path, sizeof path, "%s/%s", cdrom_dir, *ptr);
		if (stat(path, &st)) {
			audio_on_default_dir = 0;
			break;
		}
	}
	if (audio_on_default_dir)
		return;
	/* try /run/user/1000/gvfs */
	strncpy0(dev, cdrom_dev, sizeof dev);
	host = basename(dev);
	snprintf(cdrom_gvfs_dir, sizeof cdrom_gvfs_dir, GVFS_PATH "cdda:host=%s", host);
	if (stat(cdrom_gvfs_dir, &st) || (st.st_mode & S_IFMT) != S_IFDIR)
		goto no_cdrom_audio;

	audio_on_cdrom = 1;
	for (const char **ptr = cdrom_tracks_paths; *ptr; ++ptr) {
		snprintf(path, sizeof path, "%s/%s", cdrom_gvfs_dir, *ptr);
		if (stat(path, &st)) {
			perror(path);
			audio_on_cdrom = 0;
			break;
		}
	}
	if (!audio_on_cdrom) {
no_cdrom_audio:
		warnx(
			"No CD-ROM audio found. If the CD-ROM is mixed (i.e. it contains both data and audio tracks), "
			"make sure it is mounted on %s", cdrom_gvfs_dir
		);
		return;
	}
}

static void handle_cdrom_error(int code)
{
	switch (code) {
	case ERR_CDROM_SUCCESS:
		break;
	case ERR_CDROM_MOUNT:
		show_error("Fatal error", "Could not traverse mounted partitions");
		break;
	case ERR_CDROM_NODRIVE:
		show_error("CD-ROM not found", "Insert the CD-ROM and try again");
		break;
	case ERR_CDROM_FSCK:
		show_error("CD-ROM corrupt", "Required files are missing on CD-ROM");
		break;
	default:
		show_error("CD-ROM error", "Unknown internal error");
		break;
	}
}

int ge_cdrom_init(void)
{
	int error = 1;

	if (cdrom_init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}
	cdrom_init = CDROM_INIT;

	error = cdrom_is_mounted();
	if (error) {
		handle_cdrom_error(error);
		goto fail;
	}
	cdrom_try_load_audio();

	error = 0;
fail:
	return error;
}

const char *ge_cdrom_get_music_path(unsigned id)
{
	if (id == MSC_INVALID || id > CDROM_TRACKS)
		return NULL;
	if (!audio_on_default_dir && !audio_on_cdrom)
		return NULL;
	--id;

	const char *dir = cdrom_gvfs_dir;
	if (audio_on_default_dir)
		dir = cdrom_dir;
	snprintf(cdrom_msc_path, sizeof cdrom_msc_path, "%s/%s", dir, cdrom_tracks_paths[id]);
	return cdrom_msc_path;
}

const char *ge_cdrom_absolute_path(const char *path)
{
	if (!cdrom_dir)
		return NULL;
	snprintf(cdrom_abs_path, sizeof cdrom_abs_path, "%s/%s", cdrom_dir, path);
	return cdrom_abs_path;
}
