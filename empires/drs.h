/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef DRS_H
#define DRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <genie/drs.h>

#ifndef HEADLESS
#include <SDL2/SDL_surface.h>
#endif

#include "res.h"

#ifndef HEADLESS
/**
 * Construct surface with specified palette from \a data for a \a player
 * \return whether the image is dynamic
 */
bool slp_read(SDL_Surface *surf, const struct SDL_Palette *pal, const void *data, const struct slp_frame_info *frame, unsigned player);
#endif

#ifdef __cplusplus
}
#endif

#endif
