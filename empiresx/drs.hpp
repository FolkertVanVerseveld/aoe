#pragma once

#include "fs.hpp"

#include <cstdint>

#include <string>

#include <SDL2/SDL_pixels.h>

namespace genie {

typedef uint16_t res_id; /**< symbolic alias for win32 rc resource stuff */

struct IO_DrsHdr final {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend;

	bool good() const;
};

/** Raw uncompressed game asset. */
struct IO_DrsItem final {
	uint32_t id;     /**< Unique resource identifier. */
	uint32_t offset; /**< Raw file offset. */
	uint32_t size;   /**< Size in bytes. */
};

enum DrsType {
	binary = 0x62696e61,
	shape  = 0x73687020,
	slp    = 0x736c7020,
	wave   = 0x77617620,
};

/** Graphics indexed color lookup table. */
struct Palette final {
	SDL_Color tbl[256];
};

struct Background final {
	res_id bmp[3]; /**< background_files (e.g. 640x480, 800x600 and 1024x768 resources) */
	res_id pal; /**< palette_file (e.g. what color palette to use for bmp */
	res_id cursor; /**< mouse image table */
	int shade;
	res_id btn;
	res_id popup;
	int pos, col;
	uint8_t bevel[6];
	uint8_t text[6];
	uint8_t focus[6];
	uint8_t state[6];
};

class DRS final {
	Blob blob;
	const IO_DrsHdr *hdr;
public:
	DRS(const std::string &name, iofd fd, bool map);

	/** Try to load the specified game asset using the specified type and unique identifier */
	bool open_item(IO_DrsItem &item, res_id id, uint32_t type);

	Palette open_pal(res_id id);
	Background open_bkg(res_id id);
};

}
