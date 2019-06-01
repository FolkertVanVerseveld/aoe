/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Math routines
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Simple common math stuff
 */

#ifndef MATH_H
#define MATH_H

#ifdef __cplusplus
extern "C" {
#endif

static inline float saturate_f(float min, float v, float max)
{
	return v < min ? min : v > max ? max : v;
}

#ifdef __cplusplus
}

template<typename T> T saturate(T min, T v, T max)
{
	return v < min ? min : v > max ? max : v;
}
#endif

#endif
