/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine Audio subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * This is an internal header and should not be included directly.
 */

#ifndef GENIE_SFX_H
#define GENIE_SFX_H

#include "cdrom.h"

void ge_sfx_free(void);
int ge_sfx_init(void);

int ge_msc_play(unsigned id, int loops);
void ge_msc_stop(void);

#endif
