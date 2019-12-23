/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <xt/os_macros.h>
#include <xt/os.h>
#include <xt/error.h>
#include <xt/string.h>
#include <xt/hashmap.h>
#include <xt/mman.h>

#include <SDL2/SDL.h>

#if DEBUG
#define dbgf(f, ...) xtprintf(f, ## __VA_ARGS__)
#define dbgs(s) puts(s)
#else
#define dbgf(f, ...) ((void)0)
#define dbgs(s) ((void)0)
#endif

#if XT_IS_WINDOWS
#include <windows.h>
#else
#include <pwd.h>
#endif

#define TITLE "Age of Empires"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

#define CFG_NO_INTRO          0x01
#define CFG_NO_SOUND          0x02
#define CFG_NO_MUSIC          0x04
#define CFG_NO_AMBIENT        0x08
#define CFG_USE_MIDI          0x10
#define CFG_NORMAL_MOUSE      0x20
#define CFG_GAME_HELP         0x40
#define CFG_NATIVE_RESOLUTION 0x80

#define CFG_MODE_640x480 0
#define CFG_MODE_800x600 1
#define CFG_MODE_1024x768 2
#define CFG_MODE_CUSTOM 3

#define POP_MIN 25
#define POP_MAX 200

#define CFG_DIRNAME "libregenie"

#if XT_IS_WINDOWS
#define PATH_CFG_GLOBAL "C:\\ProgramData\\" CFG_DIRNAME "\\settings.ini"
#define PATH_CFG_LOCAL "C:\\Users\\%s\\AppData\\Roaming\\" CFG_DIRNAME "\\settings.ini"
#define DIR_SEP_STR "\\"
#define DIR_SEP_CH '\\'
#else
#define PATH_CFG_GLOBAL "/etc/" CFG_DIRNAME "/config"
#define PATH_CFG_LOCAL "/home/%s/.config/" CFG_DIRNAME "/config"
#define DIR_SEP_STR "/"
#define DIR_SEP_CH '/'
#endif

#ifdef PATH_MAX
#undef PATH_MAX
#endif

// no real limits, but should be sufficient...
#define PATH_MAX 4096
#define USERNAME_MAX 256

#define README_SIZE_1_0_A 50176

static struct cfg {
	unsigned options;
	unsigned screen_mode;
	unsigned limit;
	float music_volume;
	float sound_volume;
	float scroll_speed;
	unsigned width, height;
} cfg = {
	.width = DEFAULT_WIDTH,
	.height = DEFAULT_HEIGHT
};

static char cdrom[PATH_MAX];

static char *username;

#define INIT_SDL 1

// the CD-ROM has been tampered with
#define TAINTED_CDROM 1

static struct ge_state {
	unsigned init;
	SDL_Window *win;
	SDL_Renderer *ctx;
	int running;
	Uint32 timer;
	// if the engine state is tainted, no support will be provided
	unsigned tainted;
} ge_state;

static struct scr_mode {
	unsigned width, height;
	const char *name;
} scr_modes[] = {
	{640, 480, "640x480"},
	{800, 600, "800x600"},
	{1024, 768, "1024x768"},
	{800, 600, "custom"},
};

#define CFG_PATH_GLOBAL "..."

struct cfg_ini {
	const char *name;
	struct xtHashmap kv;
};

static void show_error(const char *str)
{
	fprintf(stderr, "%s\n", str);
#if XT_IS_LINUX
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", str, ge_state.win);
#else
	MessageBoxA(NULL, str, "Error", MB_OK | MB_ICONERROR);
#endif
}

#define ERR_BUFSZ 1024

static void show_error_sdl(const char *str)
{
	char buf[ERR_BUFSZ];

	if (str) {
		xtsnprintf(buf, sizeof buf, "%s: %s", str, SDL_GetError());
		show_error(buf);
	} else {
		show_error(SDL_GetError());
	}
}

#define DEFAULT_TICKS 16

static void idle(unsigned ms)
{
	(void)ms;
}

static void display(void)
{
	SDL_SetRenderDrawColor(ge_state.ctx, 0, 0, 0, 255);
	SDL_RenderClear(ge_state.ctx);
	SDL_RenderPresent(ge_state.ctx);
}

static int handle_event(SDL_Event *ev)
{
	switch (ev->type) {
	case SDL_QUIT:
		ge_state.running = 0;
		return 0;
	}

	return 1;
}

static int main_event_loop(void)
{
	SDL_Event ev;

	// initialize event state
	ge_state.running = 1;
	ge_state.timer = SDL_GetTicks();

	// ensure something has already been blitted to screen
	display();

	while (ge_state.running) {
		while (SDL_PollEvent(&ev) && handle_event(&ev))
			;

		Uint32 next = SDL_GetTicks();
		idle(next >= ge_state.timer ? next - ge_state.timer : DEFAULT_TICKS);
		display();

		ge_state.timer = next;
	}

	return 0;
}

#define hasopt(x,a,b) (!strcasecmp(x[i], a b) || !strcasecmp(x[i], a "_" b) || !strcasecmp(x[i], a " " b) || (i + 1 < argc && !strcasecmp(x[i], a) && !strcasecmp(x[i+1], b)))

/**
 * Parse command line arguments just like the original game does.
 * Options may be specified as SOMEOPTION, SOME_OPTION or SOME OPTION
 */
static int arg_parse(int argc, char **argv)
{
	int n;

	for (int i = 1; i < argc; ++i) {
		if (hasopt(argv, "no", "startup"))
			cfg.options |= CFG_NO_INTRO;
		/*
		 * These options are added for compatibility, but we
		 * just print we don't support any of them.
		 */
		else if (hasopt(argv, "system", "memory")) {
			// XXX this is not what the original game does
			fputs("System memory not supported\n", stderr);
			struct xtRAMInfo info;
			int code;

			if ((code = xtRAMGetInfo(&info)) != 0) {
				xtPerror("No system memory", code);
			} else {
				char strtot[32], strfree[32];

				xtFormatBytesSI(strtot, sizeof strtot, info.total - info.free, 2, true, NULL);
				xtFormatBytesSI(strfree, sizeof strfree, info.total, 2, true, NULL);

				printf("System memory: %s / %s\n", strtot, strfree);
			}
		} else if (hasopt(argv, "midi", "music")) {
#if XT_IS_LINUX
			fputs("Midi support unavailable\n", stderr);
#endif
		} else if (!strcasecmp(argv[i], "msync")) {
			fputs("SoundBlaster AWE not supported\n", stderr);
		} else if (!strcasecmp(argv[i], "mfill")) {
			fputs("Matrox Video adapter not supported\n", stderr);
		} else if (hasopt(argv, "no", "sound")) {
			cfg.options |= CFG_NO_SOUND;
		} else if (!strcmp(argv[i], "640")) {
			cfg.screen_mode = CFG_MODE_640x480;
		} else if (!strcmp(argv[i], "800")) {
			cfg.screen_mode = CFG_MODE_800x600;
		} else if (!strcmp(argv[i], "1024")) {
			cfg.screen_mode = CFG_MODE_1024x768;
		/* Custom option: disable changing screen resolution. */
		} else if (hasopt(argv, "no", "changeres")) {
			cfg.options = CFG_NATIVE_RESOLUTION;
		} else if (hasopt(argv, "no", "music")) {
			cfg.options |= CFG_NO_MUSIC;
		} else if (hasopt(argv, "normal", "mouse")) {
			cfg.options |= CFG_NORMAL_MOUSE;
		/* Rise of Rome options: */
		} else if (hasopt(argv, "no", "terrainsound")) {
			cfg.options |= CFG_NO_AMBIENT;
		} else if (i + 1 < argc && !strcasecmp(argv[i], "limit")) {
			n = atoi(argv[i + 1]);

			goto set_cap;
		} else if (!strcasecmp(argv[i], "limit=")) {
			n = atoi(argv[i] + strlen("limit="));

set_cap:
			if (n < POP_MIN)
				n = POP_MIN;
			else
				n = POP_MAX;

			dbgf("pop cap changed to: %d\n", n);
			cfg.limit = (unsigned)n;
		}
	}

	struct scr_mode *mode = &scr_modes[cfg.screen_mode];

	dbgf("screen mode: %s\n", mode->name);

	cfg.width = mode->width;
	cfg.height = mode->height;

	return 0;
}

void cfg_read(FILE *f, const char *path)
{
	dbgf("%s: todo: read %p from %s\n", __func__, (void*)f, path);
	(void)f;
	(void)path;
}

/**
 * Setup and process program arguments
 *
 * Program arguments are prioritized in this order:
 * - argv arguments
 * - default configuration
 * - global configuration
 *
 * These settings are read in reversed order, such that any preceding
 * item always overwrite all items below.
 */
static void cfg_init(void)
{
	char path[PATH_MAX];
	char name[USERNAME_MAX];
	FILE *f;

	/*
	 * Don't pollute the filesystem if the user has not enabled
	 * global or local configuration settings, so check first if
	 * the path we are looking for exists.
	 */
	xtstrncpy(path, PATH_CFG_GLOBAL, sizeof path);
	dbgf("global config path: %s\n", path);

	if (f = fopen(path, "r")) {
		cfg_read(f, path);
		fclose(f);
	}

	xtsnprintf(path, sizeof path, PATH_CFG_LOCAL, username);
	dbgf("local config path: %s\n", path);

	if (f = fopen(path, "r")) {
		cfg_read(f, path);
		fclose(f);
	}
}

/**
 * Just check if the proper CD-ROM has been inserted.
 * NOTE the paths are case sensitive. we could make them case
 * insensitive, but then it may not be a legitimate copy...
 */
static int cdrom_check(void)
{
	/**
	 * Just check if we can find the CD-ROM image and if it
	 * contains setupenu.dll and game/language.dll
	 */
	char path[PATH_MAX];
	struct stat st;

#if XT_IS_LINUX
	/*
	 * Try following paths in specified order:
	 * /media/cdrom
	 * /media/username/cdrom
	 * Finally, traverse every directory in /media/username
	 */

	strcpy(path, "/media/cdrom/GAME/LANGUAGE.DLL");
	if (!stat(path, &st)) {
		strcpy(cdrom, "/media/cdrom/");

		// check if the installation is legitimate, but do not
		// stop program execution if not legitimate (for now)
		strcpy(path, "/media/cdrom/README.DOC");

		goto check;
	}
#else
	path[1] = ':';
	path[2] = DIR_SEP_CH;

	for (char dl = 'D'; dl <= 'Z'; ++dl) {
		path[0] = dl;

		strcpy(path + 3, "game" DIR_SEP_STR "language.dll");
		if (stat(path, &st))
			continue;

		path[3] = '\0';
		strcpy(cdrom, path);

		// check if the installation is legitimate, but do not
		// stop program execution if not legitimate (for now)
		strcpy(path + 3, "readme.doc");

		goto check;
	}
#endif

	return 0;
check:
	if (stat(path, &st) || st.st_size != README_SIZE_1_0_A) {
		fputs("tainted cdrom\n", stderr);
		ge_state.tainted |= TAINTED_CDROM;
	}
	return 1;
}

#define DRS_FILES 5

static const char *drs_files[DRS_FILES] = {
	"Border",
	"graphics",
	"Interfac",
	"sounds",
	"Terrain"
}, *drs_files_uc[DRS_FILES] = {
	"BORDER",
	"GRAPHICS",
	"INTERFAC",
	"SOUNDS",
	"TERRAIN"
};

#define TRIBE_MAGIC "1.00tribe"
#define TRIBE_MAGIC_SIZE strlen(TRIBE_MAGIC)

struct blob {
	int fd;
	void *data;
	struct stat st;
	int prot, flags;
};

struct drs_list {
	uint32_t type;
	uint32_t offset;
	uint32_t size;
};

struct drs_item {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct drs_hdr {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend;
};

struct drs {
	struct blob blob;
	unsigned used, total;
};

static void blob_init(struct blob *b)
{
	b->fd = -1;
	b->data = NULL;
}

static int blob_open(struct blob *b, const char *path, int prot, int flags)
{
	int ret = 1, fd = -1;
	struct stat st;
	void *data = NULL;
	unsigned mode = 0;

	if (!prot)
		prot = XT_MMAN_PROT_READ | XT_MMAN_PROT_WRITE;

	if (prot & XT_MMAN_PROT_READ) {
		if (prot & XT_MMAN_PROT_WRITE)
			mode = O_RDWR;
		else
			mode = O_RDONLY;
	} else if (prot & XT_MMAN_PROT_WRITE)
		mode = O_WRONLY;

	if ((fd = open(path, mode)) == -1 || fstat(fd, &st))
		goto fail;

	if (st.st_size && (data = xtmmap(NULL, st.st_size, prot, flags, fd, 0)) == XT_MMAN_MAP_FAILED)
		goto fail;

	b->fd = fd;
	b->data = data;
	b->st = st;
	b->prot = prot;
	b->flags = flags;

	ret = 0;
fail:
	if (ret) {
		if (data)
			xtmunmap(data, st.st_size);

		if (fd != -1)
			close(fd);
	}

	return ret;
}

static void blob_close(struct blob *b)
{
	if (b->fd == -1)
		return;

	if (b->data)
		xtmunmap(b->data, b->st.st_size);

	close(b->fd);
}

static struct drs_cache {
	struct drs data[DRS_FILES];
	unsigned used, total;
	// TODO add cache
} drs_cache;

static void cdrom_path(char *buf, size_t buflen, const char *path)
{
	xtsnprintf(buf, buflen, "%s%s", cdrom, path);
}

static int drs_open(struct drs *drs, const char *path)
{
	int ret;

	dbgf("drs_open: %s\n", path);

	ret = blob_open(&drs->blob, path, XT_MMAN_PROT_READ, XT_MMAN_MAP_FILE | XT_MMAN_MAP_SHARED);
	if (ret) {
		show_error("Some DRS files are missing");
		return ret;
	}

	drs->used = drs->total = 0;

	const struct drs_hdr *hdr = drs->blob.data;
	if (memcmp(hdr->version, TRIBE_MAGIC, TRIBE_MAGIC_SIZE)) {
		show_error("Some DRS files are corrupt");
		return 1;
	}

	return 0;
}

static int drs_init(void)
{
	char path[PATH_MAX], base[PATH_MAX];

	for (unsigned i = 0; i < DRS_FILES; ++i) {
		struct drs *drs = &drs_cache.data[i];

		xtsnprintf(base, sizeof base, "GAME" DIR_SEP_STR "DATA" DIR_SEP_STR "%s.DRS", drs_files_uc[i]);
		cdrom_path(path, sizeof path, base);

		if (drs_open(drs, path))
			return 1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int ret = 1;

#if XT_IS_LINUX
	username = getpwuid(getuid())->pw_name;
#else
	username = getenv("USERNAME");
#endif

	cfg_init();

	if (arg_parse(argc, argv))
		goto fail;

	if (!cdrom_check()) {
		show_error("Please insert or mount the game CD-ROM");
		goto fail;
	}

	if (drs_init())
		goto fail;

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		show_error_sdl("Could not initialize user interface");
		goto fail;
	}

	ge_state.init |= INIT_SDL;

	if (!(ge_state.win = SDL_CreateWindow(
		TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		cfg.width, cfg.height, SDL_WINDOW_SHOWN)))
	{
		show_error_sdl("Could not create user interface");
		goto fail;
	}

	dbgf("Available render drivers: %d\n", SDL_GetNumVideoDrivers());

	if (!(ge_state.ctx = SDL_CreateRenderer(ge_state.win, -1, SDL_RENDERER_PRESENTVSYNC))) {
		show_error_sdl("Could not create rendering context");
		goto fail;
	}

	ret = main_event_loop();
fail:
	if (ge_state.init & INIT_SDL) {
		if (ge_state.ctx)
			SDL_DestroyRenderer(ge_state.ctx);
		if (ge_state.win)
			SDL_DestroyWindow(ge_state.win);

		SDL_Quit();
		ge_state.init &= ~INIT_SDL;
	}

	if (ge_state.init)
		fputs("dirty cleanup\n", stderr);

	return ret;
}
