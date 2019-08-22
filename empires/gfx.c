/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "gfx.h"
#include "../setup/def.h"

#include <stdio.h>
#include <SDL2/SDL_image.h>

TTF_Font *fnt_default;
TTF_Font *fnt_button;
TTF_Font *fnt_large;

#define BUFSZ 4096

struct gfx_cfg gfx_cfg = {0, 0, 0, 0, 0, 800, 600};

void gfx_init(void)
{
	char buf[BUFSZ];

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		panic("Could not initialize image subsystem");
	if (TTF_Init())
		panic("Could not initialize fonts");

	snprintf(buf, BUFSZ, "%s/system/fonts/" FONT_NAME_DEFAULT, path_cdrom);
	if (!(fnt_default = TTF_OpenFont(buf, FONT_PT_DEFAULT)))
		panic("Could not setup default font");

	snprintf(buf, BUFSZ, "%s/system/fonts/" FONT_NAME_BUTTON, path_cdrom);
	if (!(fnt_button = TTF_OpenFont(buf, FONT_PT_BUTTON)))
		panic("Could not setup button font");

	snprintf(buf, BUFSZ, "%s/system/fonts/" FONT_NAME_LARGE, path_cdrom);
	if (!(fnt_large = TTF_OpenFont(buf, FONT_PT_LARGE)))
		panic("Could not setup large font");
}

void gfx_free(void)
{
	TTF_CloseFont(fnt_large);
	TTF_CloseFont(fnt_button);
	TTF_CloseFont(fnt_default);

	TTF_Quit();
	IMG_Quit();
}
