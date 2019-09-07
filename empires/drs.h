/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef DRS_H
#define DRS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include <sys/types.h>

#include <SDL2/SDL_surface.h>

#include "res.h"

#define DRS_NO_REF ((uint32_t)-1)

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

struct drs_hdr {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend; // XXX go figure
};

/** Initialize SLP from \a data */
// FIXME error handling
void slp_map(struct slp *dst, const void *data);

/**
 * Construct surface with specified palette from \a data for a \a player
 * \return whether the image is dynamic
 */
bool slp_read(SDL_Surface *surf, const struct SDL_Palette *pal, const void *data, const struct slp_frame_info *frame, unsigned player);

/** Initialize Data Resources System */
void drs_init(void);
void drs_free(void);

/** Add Data Resources System archive to list of objects. */
void drs_add(const char *name);

/**
 * Get object with specified \a type and store object size in \a count.
 * \return The object or NULL when not found.
 */
const void *drs_get_item(unsigned type, unsigned id, size_t *count);

#ifdef __cplusplus
}
#endif

#endif
