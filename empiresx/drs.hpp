/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "fs.hpp"

#include <cstdint>
#include <limits.h>

#include <string>

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_render.h>

#include "base/drs.hpp"
#include "render.hpp"

struct SDL_Surface;

namespace genie {

namespace io {

static constexpr unsigned max_players = 9; // 8 + gaia

struct DrsHdr final {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend;

	bool good() const;
};

/** Raw uncompressed game asset. */
struct DrsItem final {
	uint32_t id;     /**< Unique resource identifier. */
	uint32_t offset; /**< Raw file offset. */
	uint32_t size;   /**< Size in bytes. */
};

/** Game specific image file format header. */
struct SlpHdr final {
	char version[4];
	int32_t frame_count;
	char comment[24];
};

/** Game specific image file format subimage properties. */
struct SlpFrameInfo final {
	uint32_t cmd_table_offset;
	uint32_t outline_table_offset;
	uint32_t palette_offset;
	uint32_t properties;
	int32_t width;
	int32_t height;
	int32_t hotspot_x;
	int32_t hotspot_y;
};

constexpr int16_t invalid_edge = INT16_MIN;

/** Game specific image file format subimage boundaries. */
struct SlpFrameRowEdge final {
	int16_t left_space;
	int16_t right_space;
};

}

/** Raw game specific image file wrapper. */
struct Slp final {
	io::SlpHdr *hdr;
	io::SlpFrameInfo *info;

	Slp() : hdr(NULL), info(NULL) {}

	Slp(void *data) { reset(data); }
	
	void reset(void *data) {
		hdr = (io::SlpHdr*)data;
		info = (io::SlpFrameInfo*)((char*)data + sizeof(io::SlpHdr));
	}
};

/** Graphics indexed color lookup table. */
// XXX consider refactoring to SDL_Palette
struct Palette final {
	SDL_Color tbl[256];
};

// XXX consider moving Image and AnimationTexture to another file

class Image final {
public:
	Surface surface;
	Texture texture;
	int hotspot_x, hotspot_y;

	Image();
	Image(const Image&) = delete;
	Image(Image&&) = default;

	bool load(SimpleRender &r, const Palette &pal, const Slp &slp, unsigned index, unsigned player=0);
	void draw(SimpleRender &r, int x, int y, int w=0, int h=0, int sx=0, int sy=0, bool hflip=false);
	void draw(SimpleRender &r, const SDL_Rect &bnds);
	void draw_stretch(SimpleRender &r, const SDL_Rect &to);
	void draw_stretch(SimpleRender &r, const SDL_Rect &from, const SDL_Rect &to);
};

class Animation final {
	const Slp slp;
public:
	std::unique_ptr<Image[]> images;
	res_id id;
	unsigned image_count;
	bool dynamic;

	Animation(SimpleRender &r, const Palette &pal, const Slp &slp, res_id id);

	Image &subimage(unsigned index, unsigned player=0);

	friend bool operator<(const Animation &lhs, const Animation &rhs);
};

enum DrsType {
	binary = 0x62696e61,
	shape  = 0x73687020,
	slp    = 0x736c7020,
	wave   = 0x77617620,
};

enum class MenuId {
	score = 50061,
	mission = 50060,
	lost = 50058,
	won = 50057,
	selectnav = 50055,
	start = 50051, // main is defined by SDL...
	singleplayer = 50052,
	multiplayer = 50053,
	editor = 50054,
};

struct BackgroundSettings final {
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
	const io::DrsHdr *hdr;
public:
	DRS(const std::string &name, iofd fd, bool map);

	/** Try to load the specified game asset using the specified type and unique identifier */
	bool open_item(io::DrsItem &item, res_id id, uint32_t type);

	Palette open_pal(res_id id);
	BackgroundSettings open_bkg(res_id id);
	bool open_slp(Slp &slp, res_id id);
	void *open_wav(res_id id, size_t &count);
};

}
