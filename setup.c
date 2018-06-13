/* Copyright 2018 Folkert van Verseveld. All rights resseved */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <linux/limits.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "dbg.h"

#define TITLE "Age of Empires"

#define WIDTH 640
#define HEIGHT 480

unsigned init = 0;
SDL_Window *window;
SDL_Renderer *renderer;
TTF_Font *font;

char path_cdrom[PATH_MAX];

#define BUFSZ 4096

// scratch buffer
char buf[BUFSZ];

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
	int found = 0;

	/*
	 * Try following paths in specified order:
	 * /media/cdrom
	 * /media/username/cdrom
	 * Finally, traverse every directory in /media/username
	 */

	if (find_lib_lang("/media/cdrom"))
		return 0;

	user = getlogin();
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

#define PANIC_BUFSZ 1024

void panic(const char *str) __attribute__((noreturn));

void panic(const char *str)
{
	char buf[PANIC_BUFSZ];
	snprintf(buf, PANIC_BUFSZ, "zenity --error --text=\"%s\"", str);
	system(buf);
	exit(1);
}

SDL_Surface *surf_start, *surf_reset, *surf_nuke, *surf_exit, *surf_website;
SDL_Texture *tex_start, *tex_reset, *tex_nuke, *tex_exit, *tex_website;

void init_main_menu(void)
{
	SDL_Color fg = {0, 0, 0, 255};
	// FIXME proper error handling
	surf_start = TTF_RenderText_Solid(font, "Install Age of Empires", fg);
	surf_reset = TTF_RenderText_Solid(font, "Reinstall Age of Empires", fg);
	surf_nuke = TTF_RenderText_Solid(font, "Uninstall Age of Empires", fg);
	surf_exit = TTF_RenderText_Solid(font, "Exit", fg);
	surf_website = TTF_RenderText_Solid(font, "Web Connection", fg);

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
}

void display_main_menu(void)
{
	SDL_Rect pos;
	pos.x = 0xf1; pos.y = 0x90;
	pos.w = surf_start->w; pos.h = surf_start->h;
	SDL_RenderCopy(renderer, tex_start, NULL, &pos);

	pos.x = 0xf1; pos.y = 0xba;
	pos.w = surf_reset->w; pos.h = surf_reset->h;
	SDL_RenderCopy(renderer, tex_reset, NULL, &pos);

	pos.x = 0xf1; pos.y = 0xe6;
	pos.w = surf_nuke->w; pos.h = surf_nuke->h;
	SDL_RenderCopy(renderer, tex_nuke, NULL, &pos);

	pos.x = 0xf1; pos.y = 0x10f;
	pos.w = surf_exit->w; pos.h = surf_exit->h;
	SDL_RenderCopy(renderer, tex_exit, NULL, &pos);

	pos.x = 0xf1; pos.y = 0x13a;
	pos.w = surf_website->w; pos.h = surf_website->h;
	SDL_RenderCopy(renderer, tex_website, NULL, &pos);
}

void main_event_loop(void)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderClear(renderer);

	display_main_menu();
	SDL_RenderPresent(renderer);

	SDL_Event ev;
	while (SDL_WaitEvent(&ev)) {
		switch (ev.type) {
		case SDL_QUIT:
		case SDL_KEYDOWN:
			return;
		}
	}
}

int main(void)
{
	if (!find_setup_files())
		panic("Please insert or mount the game CD-ROM");

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

	if (TTF_Init())
		panic("Could not initialize fonts");

	snprintf(buf, BUFSZ, "%s/system/fonts/arial.ttf", path_cdrom);
	font = TTF_OpenFont(buf, 20);
	if (!font)
		panic("Could not setup font");

	init_main_menu();
	main_event_loop();

	TTF_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
