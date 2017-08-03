/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine WAVE parsing
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * This is an internal header and should not be included directly.
 */

#ifndef GENIE_WAVE_H
#define GENIE_WAVE_H

#include <stddef.h>
#include <stdint.h>

#include <AL/al.h>
#include <AL/alc.h>

/* wave format chunk */
struct ckfmt {
	uint16_t format;
	uint16_t channels;
	uint32_t freq;
	uint32_t bytes;
	uint16_t align;
	uint16_t sample;
};

struct wave {
	const void *data;
	size_t size, cksz;
	struct ckfmt fmt;
	ALuint buf;
};

int wave_load(struct wave *w, const void *data, size_t size);

#endif
