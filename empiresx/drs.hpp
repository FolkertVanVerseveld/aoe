#pragma once

#include "fs.hpp"

#include <cstdint>

#include <string>

#include <SDL2/SDL_pixels.h>

namespace genie {

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

class DRS final {
	Blob blob;
	const IO_DrsHdr *hdr;
public:
	DRS(const std::string &name, iofd fd, bool map);

	/** Try to load the specified game asset using the specified type and unique identifier */
	bool open_item(IO_DrsItem &item, uint32_t id, uint32_t type);

	Palette open_pal(uint32_t id);
};

}
