#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 640

#if _WIN32
#include <Windows.h>

static void show_error(const char *title, const char *message)
{
	fprintf(stderr, "%s: %s\n", title, message);
	MessageBox(NULL, message, title, MB_ICONERROR | MB_OK);
}
#else
#include <SDL3/SDL_messagebox.h>

static void show_error(const char *title, const char *message)
{
	fprintf(stderr, "%s: %s\n", title, message);
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, NULL);
}
#endif

static void display(SDL_Renderer *gfx)
{
	SDL_SetRenderDrawColor(gfx, 100, 40, 20, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(gfx);

	SDL_SetRenderDrawColor(gfx, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderDebugText(gfx, 32, 32, "The pope uses dope");

	SDL_RenderPresent(gfx);
}

int main(int argc, char **argv)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		show_error("SDL_Init failed", SDL_GetError());
		return 1;
	}

	int ret = 1;
	SDL_Window *win = SDL_CreateWindow("Open Age of Empires Remake SDL3", DEFAULT_WIDTH, DEFAULT_HEIGHT, 0);
	SDL_Renderer *gfx = NULL;
	
	if (!win) {
		show_error("SDL_CreateWindow failed", SDL_GetError());
		goto fail;
	}

	unsigned count = SDL_GetNumRenderDrivers();
	for (unsigned i = 0; i < count; ++i) {
		printf("driver %u: %s\n", i, SDL_GetRenderDriver(i));
	}

	gfx = SDL_CreateRenderer(win, "software");
	if (!gfx) {
		show_error("SDL_CreateRenderer failed", SDL_GetError());
		goto fail;
	}

	if (!SDL_SetRenderVSync(gfx, 1))
		// log and ignore as the game would still work, just looks uglier
		fprintf(stderr, "Cannot change vsync: %s\n", SDL_GetError());

	display(gfx);

	for (int running = 1; running;) {
		SDL_Event event;
		SDL_WaitEvent(&event);

		switch (event.type) {
		case SDL_EVENT_QUIT:
			running = 0;
			break;
		}
	}

	ret = 0;
fail:
	if (gfx)
		SDL_DestroyRenderer(gfx);
	if (win)
		SDL_DestroyWindow(win);

	SDL_Quit();
	return ret;
}