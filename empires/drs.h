/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef DRS_H
#define DRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <sys/types.h>

#define DRS_BACKGROUND_MAIN 50051
#define DRS_BACKGROUND_SINGLEPLAYER 50052
#define DRS_BACKGROUND_MULTIPLAYER 50053
#define DRS_BACKGROUND_SCENARIO 50054
#define DRS_BACKGROUND_VICTORY 50057
#define DRS_BACKGROUND_DEFEAT 50058
#define DRS_BACKGROUND_ACHIEVEMENTS 50061

#define DRS_BACKGROUND_GAME_0 50006
#define DRS_BACKGROUND_GAME_1 50007
#define DRS_BACKGROUND_GAME_2 50008
#define DRS_BACKGROUND_GAME_3 50009

#define DRS_NO_REF ((uint32_t)-1)

#define DRS_MAIN_PALETTE 50500

// User interface stuff
#define DRS_MENU_BAR_640_480_0 50733
#define DRS_MENU_BAR_640_480_1 50734
#define DRS_MENU_BAR_640_480_2 50735
#define DRS_MENU_BAR_640_480_3 50736
#define DRS_MENU_BAR_800_600_0 50737
#define DRS_MENU_BAR_800_600_1 50738
#define DRS_MENU_BAR_800_600_2 50739
#define DRS_MENU_BAR_800_600_3 50740
#define DRS_MENU_BAR_1024_768_0 50741
#define DRS_MENU_BAR_1024_768_1 50742
#define DRS_MENU_BAR_1024_768_2 50743
#define DRS_MENU_BAR_1024_768_3 50744

#define DRS_BUILDINGS1 50704
#define DRS_BUILDINGS2 50705
#define DRS_BUILDINGS3 50706
#define DRS_BUILDINGS4 50707
#define DRS_UNITS 50730

#define DRS_HEALTHBAR 50745

#define DRS_GROUPS 50403
#define DRS_WAYPOINT 50404
#define DRS_MOVE_TO 50405

#define DRS_TASKS 50721
#define DRS_STATS 50731

// stat icons
#define ICON_STAT_WOOD 0
#define ICON_STAT_STONE 1
#define ICON_STAT_FOOD 2
#define ICON_STAT_GOLD 3
#define ICON_STAT_POPULATION 4
#define ICON_STAT_TRADE 5
#define ICON_STAT_RANGE 6
#define ICON_STAT_ATTACK 7
#define ICON_STAT_ARMOR 8
#define ICON_STAT_TIME 9
#define ICON_STAT_PIERCE_ARMOR 10

// task icons
#define ICON_REPAIR 0
#define ICON_MOVE 1
#define ICON_BUILD 2
#define ICON_STOP 3
#define ICON_STAND_GUARD 4
#define ICON_UNLOAD 5
#define ICON_WAYPOINT 6
#define ICON_GROUP 7
#define ICON_UNGROUP 8
#define ICON_UNSELECT 10
#define ICON_NEXT 11
#define ICON_ATTACK_GROUND 12
#define ICON_HEAL 13
#define ICON_CONVERT 14

// building icons
#define ICON_ACADEMY 0
#define ICON_ARCHERY1 1
#define ICON_ARCHERY2 2
#define ICON_BARRACKS1 3
#define ICON_BARRACKS2 4
#define ICON_BARRACKS3 5
#define ICON_DOCK1 6
#define ICON_DOCK2 7
#define ICON_DOCK3 8
#define ICON_SIEGE_WORKSHOP 9
#define ICON_FARM 10
#define ICON_GRANARY1 11
#define ICON_GRANARY2 12
#define ICON_TOWER3 13
#define ICON_TOWER2 14
#define ICON_HOUSE1 15
#define ICON_HOUSE2 16
#define ICON_HOUSE3 17
#define ICON_HOUSE4 18
#define ICON_GOVERNMENT1 19
#define ICON_GOVERNMENT2 20
#define ICON_MARKET1 21
#define ICON_MARKET2 22
#define ICON_MARKET3 23
#define ICON_STABLE2 24
#define ICON_STABLE1 25
#define ICON_TOWER1 26
#define ICON_WONDER 27
#define ICON_STORAGE_PIT1 28
#define ICON_STORAGE_PIT2 29
#define ICON_STOARGE_PIT3 30
#define ICON_TEMPLE1 31
#define ICON_TEMPLE2 32
#define ICON_TOWN_CENTER1 33
#define ICON_TOWN_CENTER2 34
#define ICON_TOWN_CENTER3 35
#define ICON_TOWN_CENTER4 36
#define ICON_TOWN_CENTER5 36
#define ICON_TOWN_CENTER6 37
#define ICON_MARKET 38
#define ICON_WALL1 39
#define ICON_WALL2 40
#define ICON_WALL3 41
#define ICON_WALL4 42
#define ICON_TOWER4 43

// unit icons
#define ICON_VILLAGER 0
#define ICON_PRIEST 1
#define ICON_CLUBMAN 2
#define ICON_AXEMAN 3
#define ICON_SWORDSMAN 4
#define ICON_BROAD_SWORDSMAN 5
#define ICON_BOWMAN 6
#define ICON_IMPROVED_BOWMAN 7
#define ICON_COMPOSITE_BOWMAN 8
#define ICON_BALLISTA 9
#define ICON_CHARIOT 10
#define ICON_CAVALRY 11
#define ICON_WAR_ELEPHANT 12
#define ICON_CATAPRHACT 13
#define ICON_STONE_THROWER 14
#define ICON_CATAPULT 15
#define ICON_HOPLITE 16
#define ICON_PHALANX 17
#define ICON_FISHING_BOAT 18
#define ICON_MERCHANT_BOAT 19
#define ICON_FISHING_SHIP 20
#define ICON_TRANSPORT_BOAT 21
#define ICON_SCOUT_SHIP 22
#define ICON_MERCHANT_SHIP 23
#define ICON_WAR_GALLEY 24
#define ICON_TRANSPORT_SHIP 25
#define ICON_TRIREME 26
#define ICON_LONG_SWORDSMAN 27
#define ICON_CHARIOT_ARCHER 28
#define ICON_HORSE_ARCHER 29
#define ICON_CATAPULT_TRIREME 30
#define ICON_SCOUT 31
#define ICON_HORSE 32
#define ICON_ELEPHANT 33
#define ICON_FISH 34
#define ICON_TREES 35
#define ICON_BERRY_BUSH 36
#define ICON_GAZELLE 37
#define ICON_STONE 38
#define ICON_GOLD 39
#define ICON_LION 40
#define ICON_ALLIGATOR 41
#define ICON_ARTEFACT 42
#define ICON_RUINS1 43
#define ICON_RUINS2 44
#define ICON_RUINS3 45
#define ICON_RUINS4 46
#define ICON_ELEPHANT_ARCHER 47
#define ICON_HEAVY_HORSE_ARCHER 48
#define ICON_LEGION 49
#define ICON_CENTURION 50
#define ICON_HEAVY_CATAPULT 51
#define ICON_JUGGERNAUGHT 52
#define ICON_HELOPOLIS 53
#define ICON_HEAVY_CATAPHRACT 54
#define ICON_BALLISTA_TOWER 55

#define DRS_CURSORS 51000

// Terrain tiles
#define DRS_TERRAIN_DESERT 15000

// Buildings
#define DRS_BARRACKS_BASE 229
#define DRS_BARRACKS_PLAYER 254
#define DRS_TOWN_CENTER_BASE 280
#define DRS_TOWN_CENTER_PLAYER 230
#define DRS_ACADEMY_BASE 16
#define DRS_ACADEMY_PLAYER 34

// Units
#define DRS_VILLAGER_CARRY 270
#define DRS_VILLAGER_STAND 418

// Gaia
#define DRS_BERRY_BUSH 240
#define DRS_DESERT_TREE1 463
#define DRS_DESERT_TREE2 464
#define DRS_DESERT_TREE3 465
#define DRS_DESERT_TREE4 466
#define DRS_GOLD 481
#define DRS_STONE 622

// FIXME also support big-endian machines
#define DT_BINARY 0x62696e61
#define DT_SHP    0x73687020
#define DT_SLP    0x736c7020
#define DT_WAVE   0x77617620

struct drs_list {
	uint32_t type;
	uint32_t offset;
	uint32_t size;
};

struct drs_item {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct slp_header {
	char version[4];
	int32_t frame_count;
	char comment[24];
};

struct slp_frame_info {
	uint32_t cmd_table_offset;
	uint32_t outline_table_offset;
	uint32_t palette_offset;
	uint32_t properties;
	int32_t width;
	int32_t height;
	int32_t hotspot_x;
	int32_t hotspot_y;
};

struct slp_frame_row_edge {
	uint16_t left_space;
	uint16_t right_space;
};

struct slp {
	struct slp_header *hdr;
	struct slp_frame_info *info;
};

// FIXME error handling
void slp_read(struct slp *dst, const void *data);

void drs_add(const char *name);

const void *drs_get_item(unsigned type, unsigned id, size_t *count);

void drs_init(void);
void drs_free(void);

#ifdef __cplusplus
}
#endif

#endif
