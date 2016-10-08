#include "engine.h"

#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>
#include <strings.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "xmap.h"
#include "todo.h"

int fd_langx = -1, fd_lang = -1;
char *data_langx = NULL, *data_lang = NULL;
size_t size_langx = 0, size_lang = 0;

int findfirst(const char *fname)
{
	DIR *d = NULL;
	struct dirent *entry;
	int found = -1;
	d = opendir(".");
	if (!d) goto fail;
	while ((entry = readdir(d)) != NULL) {
		if (!strcasecmp(entry->d_name, fname)) {
			found = 0;
			break;
		}
	}
fail:
	if (d) closedir(d);
	return found;
}

void eng_free(void)
{
	if (fd_langx != -1) {
		xunmap(fd_langx, data_langx, size_langx);
		fd_langx = -1;
	}
	if (fd_lang != -1) {
		xunmap(fd_lang, data_lang, size_lang);
		fd_lang = -1;
	}
}

void show_error(const char *title, const char *msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, NULL);
}

int go_fullscreen(SDL_Window *scr)
{
	SDL_Rect bounds;
	SDL_GetDisplayBounds(0, &bounds);
	SDL_SetWindowBordered(scr, SDL_FALSE);
	SDL_SetWindowPosition(scr, bounds.x, bounds.y);
	SDL_SetWindowSize(scr, bounds.w, bounds.h);
	return 0;
}
