/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Audio subsystem
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 */

#ifndef SFX_H
#define SFX_H

#ifndef _WIN32
	#include <AL/al.h>
	#include <AL/alc.h>
#endif

#include <genie/sfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SFX_BARRACKS 5022
#define SFX_TOWN_CENTER 5044
#define SFX_ACADEMY 5002

#define SFX_VILLAGER_MOVE 5006

#define SFX_VILLAGER1 5037
#define SFX_VILLAGER2 5053
#define SFX_VILLAGER3 5054
#define SFX_VILLAGER4 5158
#define SFX_VILLAGER5 5171

#define SFX_BUILDING_DESTROY1 5077
#define SFX_BUILDING_DESTROY2 5078
#define SFX_BUILDING_DESTROY3 5079

#define SFX_BUILDING_DESTROY_COUNT 3

#define SFX_VILLAGER_DESTROY1 5055
#define SFX_VILLAGER_DESTROY2 5056
#define SFX_VILLAGER_DESTROY3 5057
#define SFX_VILLAGER_DESTROY4 5058
#define SFX_VILLAGER_DESTROY5 5059
#define SFX_VILLAGER_DESTROY6 5060
#define SFX_VILLAGER_DESTROY7 5061
#define SFX_VILLAGER_DESTROY8 5062
#define SFX_VILLAGER_DESTROY9 5063
#define SFX_VILLAGER_DESTROY10 5064

#define SFX_VILLAGER_DESTROY_COUNT 10

#define SFX_TRIBE 5231

#define SFX_TRIBE_DESTROY1 5102
#define SFX_TRIBE_DESTROY2 5103
#define SFX_TRIBE_DESTROY3 5104

#define SFX_TRIBE_DESTROY_COUNT 3

#define MUS_GAME1 3
#define MUS_GAME2 4
#define MUS_GAME3 5
#define MUS_GAME4 6
#define MUS_GAME5 7
#define MUS_GAME6 8
#define MUS_GAME7 9
#define MUS_GAME8 10
#define MUS_GAME9 11
#define MUS_GAME10 12
/* alternative names */
#define MUS_CHOIRS 5

#ifdef __cplusplus
}
#endif

#endif
