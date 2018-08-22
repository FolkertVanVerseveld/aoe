/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

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

#define DRS_NO_REF ((uint32_t)-1)

#define DRS_MAIN_PALETTE 50500

// User interface stuff
#define DRS_MENU_BAR 50740

// Terrain tiles
#define DRS_TERRAIN_DESERT 15000

// Buildings
#define DRS_TOWN_CENTER_BASE 280
#define DRS_TOWN_CENTER_PLAYER 230

// Units
#define DRS_VILLAGER_CARRY 270

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
