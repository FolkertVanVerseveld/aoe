/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <string.h>

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Surface *surf_font;
SDL_Texture *tex_font;

#define TITLE "font tester"
#define WIDTH 800
#define HEIGHT 600

struct fnt_tm {
	int tm_height;
	int tm_ascent;
	int tm_descent;
	int tm_ilead;
	int tm_elead;
	int tm_avgchw;
	int tm_maxchw;
	int tm_weight;
	int tm_ohang;
	int tm_dasx;
	int tm_dasy;
	char tm_chrfst;
	char tm_chrend;
	char tm_chrdef;
	char tm_chrbrk;
	char tm_italic;
	char tm_uline;
	char tm_struck;
	char tm_paf;
	char tm_cs;
} fnt_tm;

struct fnt_cs {
	int data[2 * 256];
} fnt_cs;

int tm_read(FILE *f, struct fnt_tm *tm)
{
	unsigned short args[9];

	if (fscanf(f,
		"font metrics:\n"
		"height                 : %d\n"
		"ascent                 : %d\n"
		"descent                : %d\n"
		"internal leading       : %d\n"
		"external leading       : %d\n"
		"average character width: %d\n"
		"maximum character width: %d\n"
		"weight                 : %d\n"
		"overhang               : %d\n"
		"digitized aspect x     : %d\n"
		"digitized aspect y     : %d\n"
		/* windoze does not support %hh in scanf.... */
		"first character        : %2hX\n"
		"last character         : %2hX\n"
		"default character      : %2hX\n"
		"break character        : %2hX\n"
		"italic                 : %2hX\n"
		"underlined             : %2hX\n"
		"struck out             : %2hX\n"
		"pitch and family       : %2hX\n"
		"character set          : %2hX\n",
		&tm->tm_height,
		&tm->tm_ascent,
		&tm->tm_descent,
		&tm->tm_ilead,
		&tm->tm_elead,
		&tm->tm_avgchw,
		&tm->tm_maxchw,
		&tm->tm_weight,
		&tm->tm_ohang,
		&tm->tm_dasx,
		&tm->tm_dasy,
		&args[0],
		&args[1],
		&args[2],
		&args[3],
		&args[4],
		&args[5],
		&args[6],
		&args[7],
		&args[8]) != 20)
		return 0;

	tm->tm_chrfst = args[0];
	tm->tm_chrend = args[1];
	tm->tm_chrdef = args[2];
	tm->tm_chrbrk = args[3];
	tm->tm_italic = args[4];
	tm->tm_uline = args[5];
	tm->tm_struck = args[6];
	tm->tm_paf = args[7];
	tm->tm_cs = args[8];
	return 1;
}

int cs_read(FILE *f, struct fnt_cs *cs, unsigned count)
{
	unsigned dummy;

	for (unsigned i = 0; i < count; ++count)
		if (fscanf(f, "%2X: %d,%d\n", &dummy, &cs->data[2 * i + 0], &cs->data[2 * i + 1]) != 3)
			return 0;

	return 1;
}

void draw_textlen(int x, int y, const char *text, unsigned count)
{
	SDL_Rect ren_pos, tex_pos;
	int tx = x, ty = y;

	for (const unsigned char *ptr = (const unsigned char*)text; count; ++ptr, --count) {
		int ch = *ptr;

		// treat invalid characters as break
		if (ch < fnt_tm.tm_chrfst || ch > fnt_tm.tm_chrend)
			ch = fnt_tm.tm_chrbrk;

		if (ch == '\r')
			continue;

		if (ch == '\n') {
			tx = x;
			ty += fnt_tm.tm_height;
		}

		ren_pos.x = tx;
		ren_pos.y = ty;
	}
}

void draw_text(int x, int y, const char *str)
{
	draw_textlen(x, y, str, strlen(str));
}

void display(void)
{
	SDL_SetRenderDrawColor(renderer, 0xf0, 0xf0, 0xf0, 0);
	SDL_RenderClear(renderer);

	/* TODO render image */
	SDL_RenderCopy(renderer, tex_font, NULL, NULL);

	SDL_RenderPresent(renderer);
}

int mainloop(void)
{
	while (1) {
		SDL_Event ev;

		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_KEYDOWN:
			case SDL_QUIT: return 0;
			}
		}

		SDL_Delay(50);

		display();
	}

	fputs("unexpected end of mainloop\n", stderr);
	return 1;
}

int main(int argc, char **argv)
{
	int ret = 1;
	FILE *f = NULL;

	if (argc != 3) {
		fprintf(stderr, "usage: %s bitmap glyph_file\n", argc > 0 ? argv[0] : "font");
		goto fail;
	}

	if (!(f = fopen(argv[2], "r"))) {
		perror(argv[2]);
		goto fail;
	}

	if (!tm_read(f, &fnt_tm)) {
		fprintf(stderr, "%s: bad glyph file\n", argv[2]);
		goto fail;
	}

	if (!cs_read(f, &fnt_cs, fnt_tm.tm_chrend - fnt_tm.tm_chrfst + 1)) {
		fprintf(stderr, "%s: bad glyph data\n", argv[2]);
		goto fail;
	}

	fclose(f);
	f = NULL;

	if (SDL_Init(SDL_INIT_VIDEO)) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		goto fail;
	}

	if (!(window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN))) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());

		goto fail_sdl;
	}

	/* Create default renderer and don't care if it is accelerated. */
	if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC))) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		goto fail_sdl;
	}

	if (!IMG_Init(IMG_INIT_PNG)) {
		fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
		goto fail_sdl;
	}

	if (!(surf_font = IMG_Load(argv[1]))) {
		fprintf(stderr, "IMG_Load: %s: %s\n", argv[1], IMG_GetError());
		goto fail_img;
	}

	if (!(tex_font = SDL_CreateTextureFromSurface(renderer, surf_font))) {
		fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n", SDL_GetError());
		goto fail_img;
	}

	ret = mainloop();

fail_img:
	if (tex_font)
		SDL_DestroyTexture(tex_font);
	if (surf_font)
		SDL_FreeSurface(surf_font);
	IMG_Quit();
fail_sdl:
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
	SDL_Quit();
fail:
	if (f)
		fclose(f);
	return ret;
}
