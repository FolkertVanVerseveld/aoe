/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/*
 * Anything image and animation related stuff goes here
 */

#pragma once

#include "drs.h"

#include <SDL2/SDL_image.h>

/**
 * Holds all current available indexed colors.
 */
class Palette final {
public:
	// XXX wrap in unique_ptr?
	SDL_Palette *pal;

	Palette() : pal(NULL) {}
	Palette(unsigned id) : pal(NULL) { open(id); }

	~Palette() {
		if (pal)
			SDL_FreePalette(pal);
	}

	void open(unsigned id);
	void set_border_color(unsigned id, unsigned col_id) const;
};

/**
 * SLP wrapper.
 */
class Image final {
public:
	SDL_Surface *surface;
	SDL_Texture *texture;

	Image();
	~Image();

	void load(
		Palette *pal, const void *data,
		const struct slp_frame_info *frame
	);

	void draw(int x, int y) const;
};

/**
 * SLP image wrapper.
 */
class AnimationTexture final {
	struct slp image;
public:
	// FIXME make private and wrap in unique_ptr
	Image *images;

	AnimationTexture() : images(NULL) {}
	AnimationTexture(Palette *pal, unsigned id) : images(NULL) { open(pal, id); }

	~AnimationTexture();

	void open(Palette *pal, unsigned id);
	void draw(int x, int y, unsigned index) const;
};
