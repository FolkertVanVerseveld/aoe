/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Replicated installer and game launcher
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Custom setup that looks like the original one
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <linux/limits.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_surface.h>

#include <libpe/pe.h>

#include "bmp.h"
#include "dbg.h"
#include "def.h"
#include "res.h"

#define TITLE "Age of Empires"

// Original website is dead, so use archived link
#define WEBSITE "http://web.archive.org/web/19980120120129/https://www.microsoft.com/games/empires"

#define WIDTH 640
#define HEIGHT 480

/* SDL handling */

unsigned init = 0;
SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

/* Paths to CDROM and wine installed directory */

char path_cdrom[PATH_MAX];
char path_wine[PATH_MAX];

int has_wine = 0;
int game_installed;

#define BUFSZ 4096

/* Resource IDs */

#define BMP_MAIN_BKG 0xA2
#define BMP_MAIN_BTN 0xD1

#define STR_PLAY_GAME 0x15
#define STR_INSTALL_GAME 0x16
#define STR_RESET_GAME 0x1A
#define STR_NUKE_GAME 0x1B
#define STR_EXIT_SETUP 0x51
#define STR_OPEN_WEBSITE 0x3C

#define IMG_BTN_DISABLED 0
#define IMG_BTN_NORMAL   1
#define IMG_BTN_FOCUS    2
#define IMG_BTN_CLICKED  3

// scratch buffer
char buf[BUFSZ];

static struct pe_lib lib_lang;

// XXX peres -x /media/methos/AOE/setupenu.dll

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
	return found;
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

	pwd = getpwuid(getuid());
	user = pwd->pw_name;

	snprintf(path, PATH_MAX, "/home/%s/.wine/system.reg", user);
	if ((fd = open(path, O_RDONLY)) == -1)
		return 0;
	has_wine = 1;
	close(fd);

	snprintf(path, PATH_MAX, "/home/%s/.wine/drive_c/Program Files (x86)/Microsoft Games/Age of Empires/Empires.exe", user);
	if ((fd = open(path, O_RDONLY)) == -1)
		return 0;
	close(fd);
	snprintf(path_wine, PATH_MAX, "/home/%s/.wine/drive_c/Program Files (x86)/Microsoft Games/Age of Empires", user);

	return 1;
}

int load_lib_lang(void)
{
	snprintf(buf, BUFSZ, "%s/setupenu.dll", path_cdrom);
	return pe_lib_open(&lib_lang, buf);
}

// XXX throw away surfaces?

SDL_Surface *surf_start, *surf_reset, *surf_nuke, *surf_exit, *surf_website;
SDL_Texture *tex_start, *tex_reset, *tex_nuke, *tex_exit, *tex_website;

SDL_Surface *surf_bkg, *surf_btn;
SDL_Texture *tex_bkg, *tex_btn;

struct menu_item {
	SDL_Rect pos;
	int x, y;
	unsigned image;
	unsigned id, format;
	SDL_Surface **surf;
	SDL_Texture **tex;
} menu_items[] = {
	{{0xf1, 0x90 , 0x1b8, 0xb7 }, 197, 138, 2, STR_INSTALL_GAME, 0x10, &surf_start, &tex_start},
	{{0xf1, 0xba , 0x1b0, 0xcd }, 197, 180, 0, STR_RESET_GAME, 0, &surf_reset, &tex_reset},
	{{0xf1, 0xe6 , 0x1b1, 0xf9 }, 197, 223, 0, STR_NUKE_GAME, 0, &surf_nuke, &tex_nuke},
	{{0xf1, 0x10f, 0x1b8, 0x136}, 197, 265, 1, STR_EXIT_SETUP, 0x10, &surf_exit, &tex_exit},
	{{0xf1, 0x13a, 0x1b8, 0x161}, 197, 307, 1, STR_OPEN_WEBSITE, 0x10, &surf_website, &tex_website},
};

unsigned menu_option = 0;


void init_main_menu(void)
{
	SDL_Color fg = {0, 0, 0, 255};

	game_installed = find_wine_installation();
	if (has_wine)
		dbgs("wine detected");

	if (game_installed) {
		dbgs("windows installation detected");
		// enable reset and nuke buttons
		menu_items[1].image = 1;
		menu_items[2].image = 1;
	} else {
		// disable reset and nuke buttons
		menu_items[1].image = 0;
		menu_items[2].image = 0;
	}

	// FIXME proper error handling
	load_string(&lib_lang, game_installed ? STR_PLAY_GAME : STR_INSTALL_GAME, buf, BUFSZ);
	surf_start = TTF_RenderText_Solid(font, buf, fg);
	load_string(&lib_lang, STR_RESET_GAME, buf, BUFSZ);
	surf_reset = TTF_RenderText_Solid(font, buf, fg);
	load_string(&lib_lang, STR_NUKE_GAME, buf, BUFSZ);
	surf_nuke = TTF_RenderText_Solid(font, buf, fg);
	load_string(&lib_lang, STR_EXIT_SETUP, buf, BUFSZ);
	surf_exit = TTF_RenderText_Solid(font, buf, fg);
	load_string(&lib_lang, STR_OPEN_WEBSITE, buf, BUFSZ);
	surf_website = TTF_RenderText_Solid(font, buf, fg);

	tex_start = SDL_CreateTextureFromSurface(renderer, surf_start);
	assert(tex_start);
	tex_reset = SDL_CreateTextureFromSurface(renderer, surf_reset);
	assert(tex_reset);
	tex_nuke = SDL_CreateTextureFromSurface(renderer, surf_nuke);
	assert(tex_nuke);
	tex_exit = SDL_CreateTextureFromSurface(renderer, surf_exit);
	assert(tex_exit);
	tex_website = SDL_CreateTextureFromSurface(renderer, surf_website);
	assert(tex_website);

	void *data;
	size_t size;
	int ret;
	SDL_RWops *mem;
	Uint32 colkey;

	ret = load_bitmap(&lib_lang, BMP_MAIN_BKG, &data, &size);
	assert(ret == 0);

	mem = SDL_RWFromMem(data, size);
	surf_bkg = SDL_LoadBMP_RW(mem, 1);
	tex_bkg = SDL_CreateTextureFromSurface(renderer, surf_bkg);
	assert(tex_bkg);

	ret = load_bitmap(&lib_lang, BMP_MAIN_BTN, &data, &size);
	assert(ret == 0);
	mem = SDL_RWFromMem(data, size);
	surf_btn = SDL_LoadBMP_RW(mem, 1);
	// enable transparent pixels
	dbgf("format: %X\n", SDL_PIXELTYPE(surf_btn->format->format));
	assert(surf_btn);
	colkey = SDL_MapRGB(surf_btn->format, 0xff, 0, 0xff);
	SDL_SetColorKey(surf_btn, 1, colkey);
	tex_btn = SDL_CreateTextureFromSurface(renderer, surf_btn);
	assert(tex_btn);
}

void display_main_menu(void)
{
	SDL_Rect pos, img;
	pos.x = 0; pos.y = 0;
	pos.w = surf_bkg->w; pos.h = surf_bkg->h;
	SDL_RenderCopy(renderer, tex_bkg, NULL, &pos);

	for (unsigned i = 0; i < ARRAY_SIZE(menu_items); ++i) {
		struct menu_item *item = &menu_items[i];
		pos.x = item->x; pos.y = item->y;
		pos.w = surf_btn->w; pos.h = surf_btn->h / 4;
		img.x = 0; img.y = item->image * surf_btn->h / 4;
		img.w = surf_btn->w; img.h = surf_btn->h / 4;
		SDL_RenderCopy(renderer, tex_btn, &img, &pos);
	}

	for (unsigned i = 0; i < ARRAY_SIZE(menu_items); ++i) {
		struct menu_item *item = &menu_items[i];
		item->pos.w = (*item->surf)->w;
		item->pos.h = (*item->surf)->h;
		SDL_RenderCopy(renderer, *item->tex, NULL, &item->pos);
	}
}

static int button_down = 0;

static void main_btn_install_or_play(void)
{
	char path[PATH_MAX];

	if (!game_installed)
		// FIXME stub
		return;

	// TODO show launching menu

	snprintf(path, PATH_MAX, "wine '%s/Empires.exe'", path_wine);
	system(path);
}

int menu_btn_click(void)
{
	switch (menu_option) {
	case 0:
		main_btn_install_or_play();
		break;
	case 1:
	case 2:
		// FIXME stub
		break;
	case 3:
		return -1;
	case 4:
		system("xdg-open '" WEBSITE "'");
		break;
	}
	button_down = 0;
	menu_items[menu_option].image = 2;
	return 1;
}

unsigned mouse_find_button(int x, int y)
{
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(menu_items); ++i) {
		struct menu_item *item = &menu_items[i];
		if (x >= item->x && x < item->x + surf_btn->w &&
			y >= item->y && y < item->y + surf_btn->h / 4)
		{
			break;
		}
	}
	return i;
}

int mouse_move(const SDL_MouseMotionEvent *ev)
{
	unsigned old_option, index;

	old_option = menu_option;
	index = mouse_find_button(ev->x, ev->y);

	if (index < ARRAY_SIZE(menu_items)) {
		if (menu_items[index].image && !button_down)
			menu_option = index;
		else if (button_down && index == menu_option && menu_items[index].image != IMG_BTN_CLICKED) {
			menu_items[index].image = IMG_BTN_CLICKED;
			return 1;
		}
	} else if (!button_down && menu_items[menu_option].image != IMG_BTN_DISABLED && menu_items[menu_option].image != IMG_BTN_NORMAL) {
		menu_items[menu_option].image = IMG_BTN_FOCUS;
		return 1;
	}

	if (button_down)
		return 0;

	if (old_option != menu_option) {
		//dbgf("mouse_move (%d,%d): %u\n", ev->x, ev->y, menu_option);
		menu_items[old_option].image = IMG_BTN_NORMAL;
		menu_items[menu_option].image = IMG_BTN_FOCUS;
		return 1;
	}
	return 0;
}

int mouse_down(const SDL_MouseButtonEvent *ev)
{
	if (ev->button != SDL_BUTTON_LEFT)
		return 0;
	if (mouse_find_button(ev->x, ev->y) == menu_option) {
		menu_items[menu_option].image = IMG_BTN_CLICKED;
		button_down = 1;
		return 1;
	}
	return 0;
}

int mouse_up(const SDL_MouseButtonEvent *ev)
{
	unsigned index;

	if (ev->button != SDL_BUTTON_LEFT)
		return 0;
	if (mouse_find_button(ev->x, ev->y) == menu_option)
		return menu_btn_click();
	button_down = 0;

	index = mouse_find_button(ev->x, ev->y);
	if (index < ARRAY_SIZE(menu_items) && index != menu_option) {
		menu_items[menu_option].image = IMG_BTN_NORMAL;
		if (menu_items[index].image != IMG_BTN_DISABLED) {
			menu_option = index;
			menu_items[index].image = IMG_BTN_FOCUS;
		}
	} else
		menu_items[menu_option].image = IMG_BTN_FOCUS;
	return 1;
}

int keydown(const SDL_Event *ev)
{
	unsigned virt;
	unsigned old_option;

	virt = ev->key.keysym.sym;

	old_option = menu_option;

	switch (virt) {
	case SDLK_DOWN:
		if (button_down)
			break;
		do {
			menu_option = (menu_option + 1) % ARRAY_SIZE(menu_items);
		} while (menu_items[menu_option].image == IMG_BTN_DISABLED);
		break;
	case SDLK_UP:
		if (button_down)
			break;
		do {
			menu_option = (menu_option + ARRAY_SIZE(menu_items) - 1) % ARRAY_SIZE(menu_items);
		} while (menu_items[menu_option].image == IMG_BTN_DISABLED);
		break;
	case '\r':
	case '\n':
		button_down = 1;
		return menu_btn_click();
	case ' ':
		if (menu_option < ARRAY_SIZE(menu_items) && menu_items[menu_option].image != IMG_BTN_DISABLED) {
			menu_items[menu_option].image = IMG_BTN_CLICKED;
			button_down = 1;
			return 1;
		}
	}

	if (old_option != menu_option) {
		menu_items[old_option].image = IMG_BTN_NORMAL;
		menu_items[menu_option].image = IMG_BTN_FOCUS;
		return 1;
	}
	return 0;
}

int keyup(const SDL_Event *ev)
{
	unsigned virt = ev->key.keysym.sym;

	switch (virt) {
	case ' ':
		return menu_btn_click();
	}
	return 0;
}

void update_screen(void)
{
	display_main_menu();
	SDL_RenderPresent(renderer);
}

void main_event_loop(void)
{
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	update_screen();

	SDL_Event ev;
	int code;
	while (SDL_WaitEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
			return;
		case SDL_KEYUP:
			if ((code = keyup(&ev)) < 0)
				return;
			else if (code > 0)
				update_screen();
			break;
		case SDL_KEYDOWN:
			if ((code = keydown(&ev)) < 0)
				return;
			else if (code > 0)
				update_screen();
			break;
		case SDL_MOUSEMOTION:
			if (mouse_move(&ev.motion))
				update_screen();
			break;
		case SDL_MOUSEBUTTONDOWN:
			if (mouse_down(&ev.button))
				update_screen();
			break;
		case SDL_MOUSEBUTTONUP:
			if ((code = mouse_up(&ev.button)) < 0)
				return;
			else if (code > 0)
				update_screen();
			break;
		}
	}
}

int main(void)
{
	if (!find_setup_files())
		panic("Please insert or mount the game CD-ROM");
	if (load_lib_lang())
		panic("CD-ROM files are corrupt");

	if (SDL_Init(SDL_INIT_VIDEO))
		panic("Could not initialize user interface");

	if (!(window = SDL_CreateWindow(
		TITLE, SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
		SDL_WINDOW_BORDERLESS | SDL_WINDOW_SHOWN)))
	{
		panic("Could not create user interface");
	}

	dbgf("Available render drivers: %d\n", SDL_GetNumVideoDrivers());

	// Create default renderer and don't care if it is accelerated.
	if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC)))
		panic("Could not create rendering context");

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		panic("Could not initialize image subsystem");
	if (TTF_Init())
		panic("Could not initialize fonts");

	snprintf(buf, BUFSZ, "%s/system/fonts/arial.ttf", path_cdrom);
	font = TTF_OpenFont(buf, 18);
	if (!font)
		panic("Could not setup font");

	init_main_menu();
	main_event_loop();

	TTF_Quit();
	IMG_Quit();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
