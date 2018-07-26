/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "gfx.h"
#include "../setup/def.h"

#include <SDL2/SDL_image.h>

TTF_Font *fnt_default;

#define BUFSZ 4096

void gfx_init(void)
{
	char buf[BUFSZ];

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		panic("Could not initialize image subsystem");
	if (TTF_Init())
		panic("Could not initialize fonts");

	snprintf(buf, BUFSZ, "%s/system/fonts/" FONT_NAME_DEFAULT, path_cdrom);
	if (!(fnt_default = TTF_OpenFont(buf, FONT_PT_DEFAULT)))
		panic("Could not setup font");
}

void gfx_free(void)
{
	TTF_Quit();
	IMG_Quit();
}
