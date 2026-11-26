#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <stdio.h>

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 640

#define SETUP_WIDTH 270
#define SETUP_HEIGHT 120

#define SETUP_MENU_WIDTH 640
#define SETUP_MENU_HEIGHT 480

enum MenuState {
	kMenu_setup_init, // 0
	kMenu_setup_start // 1
} menu_state;

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

struct CachedText {
	SDL_Color col;
	SDL_Texture *tex;
} txt_setup;

static void CachedTextFree(struct CachedText *txt)
{
	if (txt->tex)
		SDL_DestroyTexture(txt->tex);
}

static void display(SDL_Renderer *gfx)
{
	SDL_SetRenderDrawColor(gfx, 128, 128, 128, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(gfx);

	if (txt_setup.tex) {
		float w, h;
		SDL_GetTextureSize(txt_setup.tex, &w, &h);
		SDL_FRect dst = { 68, 40, w, h };

		SDL_RenderTexture(gfx, txt_setup.tex, NULL, &dst);
	} else {
		SDL_SetRenderDrawColor(gfx, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderDebugText(gfx, 68, 40, "Setup is initializing. This may take a few minutes");
	}

	SDL_RenderPresent(gfx);
}

int main(int argc, char **argv)
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		show_error("SDL_Init failed", SDL_GetError());
		return 1;
	}

	int ret = 1;
	SDL_Window *win = SDL_CreateWindow("Open Age of Empires Remake SDL3", SETUP_WIDTH, SETUP_HEIGHT, 0);
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

	SDL_SetRenderLogicalPresentation(gfx, SETUP_WIDTH, SETUP_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

	if (!TTF_Init()) {
		show_error("TTF_Init failed", SDL_GetError());
		goto fail;
	}

	TTF_Font *fnt = TTF_OpenFont("C:\\Windows\\Fonts\\SSERIFE.FON", 10.0f);// SSERIFE.FON // segoeui.ttf", 12.0f);
	if (!fnt) {
		show_error("TTF_OpenFont failed", SDL_GetError());
		goto fail;
	}

	SDL_Surface *surf = TTF_RenderText_Blended_Wrapped(fnt, "Setup is initializing. This may take a few minutes", 0, (SDL_Color) { 0, 0, 0, SDL_ALPHA_OPAQUE }, SETUP_WIDTH - 68);
	if (!surf) {
		show_error("Cannot render text", SDL_GetError());
		goto fail;
	}

	txt_setup.tex = SDL_CreateTextureFromSurface(gfx, surf);

	float w, h;
	SDL_GetTextureSize(txt_setup.tex, &w, &h);

	if (!txt_setup.tex)
		show_error("Cannot create texture", SDL_GetError());

	SDL_DestroySurface(surf);

	TTF_CloseFont(fnt);

	display(gfx);

	for (int running = 1; running;) {
		SDL_Event event;
		SDL_WaitEvent(&event);

		switch (event.type) {
		case SDL_EVENT_QUIT:
			running = 0;
			break;
		case SDL_EVENT_KEY_UP:
			if (event.key.scancode == SDL_SCANCODE_F11) {
				Uint64 v = SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN;
				SDL_SetWindowFullscreen(win, v == 0);
				display(gfx);
			}
			break;
		}
	}

	ret = 0;

fail_ttf:
	CachedTextFree(&txt_setup);

	TTF_Quit();
fail:
	if (gfx)
		SDL_DestroyRenderer(gfx);
	if (win)
		SDL_DestroyWindow(win);

	SDL_Quit();
	return ret;
}