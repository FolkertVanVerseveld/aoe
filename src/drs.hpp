/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#pragma once

#include "types.hpp"
#include "fs.hpp"

#include <cassert>

#include <memory>
#include <vector>
#include <variant>
#include <optional>
#include <string>

#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_surface.h>

namespace genie {

extern std::string game_dir;

// low-level legacy stuff, don't touch this
namespace io {

static constexpr unsigned max_players = 9; // 8 + gaia

struct DrsHdr final {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend;

	bool good() const;
};

/** Array of resource items. */
struct DrsList final {
	uint32_t type;   /**< Data Resource format. */
	uint32_t offset; /**< Raw offset in archive. */
	uint32_t size;   /**< Size in bytes. */
};

/** Raw game asset. */
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

static constexpr int16_t invalid_edge = INT16_MIN;

/** Game specific image file format subimage boundaries. */
struct SlpFrameRowEdge final {
	int16_t left_space;
	int16_t right_space;
};

/** Raw game specific image file wrapper. */
struct Slp final {
	io::SlpHdr *hdr;
	io::SlpFrameInfo *info;
	size_t size;

	Slp() : hdr(NULL), info(NULL), size(0) {}
	Slp(void *data, size_t size) { reset(data, size); }

	void reset(void *data, size_t size);
};

}

// id tables

/** HUD unit statistics */
enum class StatId {
	wood,
	stone,
	food,
	gold,
	population,
	trade,
	range,
	attack,
	armor,
	time,
	piece_armor,
};

/** Unit states */
enum class TaskId {
	repair,
	move,
	build,
	stop,
	stand_guard,
	unload,
	waypoint,
	group,
	ungroup,
	unselect,
	next,
	attack_ground,
	heal,
	convert,
};

/** All controllable buildings. Note that some civilizations use different icons. */
enum class BuildingId {
	academy,
	archery1,
	archery2,
	barracks1,
	barracks2,
	barracks3,
	dock1,
	dock2,
	dock3,
	siege_workshop,
	farm,
	granary1,
	granary2,
	tower3,
	tower2,
	storage_pit1,
	storage_pit2,
	storage_pit3,
	temple1,
	temple2,
	town_center1,
	town_center2,
	town_center3,
	town_center4,
	town_center5,
	town_center6,
	market,
	wall1,
	wall2,
	wall3,
	wall4,
	tower4,
};

/* List of all dynamic gaia and human units. */
enum class UnitId {
	villager,
	priest,
	clubman,
	axeman,
	swordsman,
	broad_swordsman,
	bowman,
	improved_bowman,
	composite_bowman,
	ballista,
	chariot,
	cavalry,
	war_elephant,
	cataphract,
	stone_thrower,
	catapult,
	hoplite,
	phalanx,
	fishing_boat,
	merchant_boat,
	fishing_ship,
	transport_boat,
	scout_ship,
	merchant_ship,
	war_galley,
	transport_ship,
	trireme,
	long_swordsman,
	chariot_archer,
	horse_archer,
	catapult_trireme,
	scout,
	horse,
	elephant,
	fish,
	trees,
	berry_bush,
	gazelle,
	stone,
	gold,
	lion,
	alligator,
	artefact,
	ruins1,
	ruins2,
	ruins3,
	ruins4,
	elephant_archer,
	heavy_horse_archer,
	legion,
	centurion,
	heavy_catapult,
	juggernaught,
	helopolis,
	heavy_cataphract,
	ballista_tower,
};

/** Raw data resource set identifiers */
enum class DrsId {
	// placeholders
	empty3x3 = 229,
	// buildings: each building must have a base and player layer.
	// the player layer is the one that determines its dimensions
	barracks_base = 254,
	town_center_base = 280,
	town_center_player = 230,
	// gaia stuff
	desert_tree = 463,
	// units
	villager_idle = 418,
	clubman_stand = 425,
	/* dialogs/screen menus */
	menu_start = 50051,
	menu_editor = 50054,
	/* sfx */
	game = 5036,
	button4 = 50300,
	chat = 50302,
	error = 50303,
	win = 50320,
	lost = 50321,
	/* hud */
	palette = 50500,
	// icons
	buildings1 = 50704,
	buildings2 = 50705,
	buildings3 = 50706,
	buildings4 = 50707,

	healthbar = 50745,

	groups = 50403,
	waypoint = 50404,
	move_to = 50405,
	// hud labels
	tasks = 50721,
	stats = 50731,
};

/** Supported DRS file formats */
enum class DrsType {
	binary,
	shape,
	slp,
	wave,
	max,
};

static constexpr uint32_t drs_type_bin = 0x62696e61;
static constexpr uint32_t drs_type_shp = 0x73687020;
static constexpr uint32_t drs_type_slp = 0x736c7020;
static constexpr uint32_t drs_type_wav = 0x77617620;

static constexpr char *JASC_PAL = "JASC-PAL\r\n0100\r\n";

extern const uint32_t drs_types[4];

class DRS;

struct DrsItem final {
	void *data;
	uint32_t size;
};

class DrsList final {
	const DRS &drs;
	unsigned pos;
public:
	uint32_t type;
	uint32_t offset;
	uint32_t size;

	DrsList(const DRS &ref, unsigned pos);
	DrsList(const DrsList&) = default;

	const io::DrsItem &operator[](unsigned pos) const noexcept;

	constexpr bool empty() const noexcept { return size == 0; }
};

struct DialogSettings final {
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

/** High-level representation of color fields in DialogSettings. */
struct DialogColors final {
	SDL_Color bevel[6], text[6], focus[6], state[6];
	int shade;
};

class DRS;

/** Most generic graphical asset loaded as shape, shape list or plain bitmap */
class Image final {
public:
	res_id id;
	unsigned idx;
	std::variant<std::nullopt_t, std::vector<uint8_t>, std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>> surf;
	SDL_Rect bnds; // x,y are hotspot x and y. w and h are surf->w, surf->h
	bool dynamic;

	Image(); // construct invalid image
	Image(res_id id, unsigned subimage, const DRS &drs, size_t size, unsigned player=0);
	Image(res_id id, const io::Slp &slp, unsigned idx, size_t size, unsigned player=0);

	Image(const Image&) = delete;
	Image(Image&&) = default;

	bool load(const io::Slp &slp, unsigned idx, unsigned player=0);

	friend bool operator<(const Image &lhs, const Image &rhs);
};

class Animation final {
public:
	const io::Slp slp;
	size_t size;
	std::unique_ptr<Image[]> images;
	res_id id;
	unsigned image_count;
	bool dynamic;

	Animation(res_id id, const DRS &drs);
	Animation(const Animation&) = delete;
	Animation(Animation&&) = default;

	const Image &subimage(unsigned index, unsigned player=0) const;

	friend bool operator<(const Animation &lhs, const Animation &rhs);
};

class Dialog final {
public:
	res_id id;
	const DRS &drs;
	DialogSettings cfg;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal;
	int bkgmode;
	std::unique_ptr<Animation> bkganim;

	Dialog(res_id id, DialogSettings &&cfg, const DRS &drs, int bkgmode=-1);
	Dialog(const Dialog&) = delete;
	Dialog(Dialog&&) = default;

	void set_bkg(int bkgmode);

	DialogColors colors();
};

class DRS final {
public:
	Blob blob;
	const io::DrsHdr *hdr;

	DRS(const std::string &name, iofd fd, bool map);

	/** Try to load the specified game asset using the specified type and unique identifier */
	bool open_item(DrsItem &item, res_id id, DrsType type) const noexcept;

	Dialog open_dlg(res_id id) const;
	io::Slp open_slp(res_id id) const;
	Image open_image(res_id id) const;
	Animation open_anim(res_id id) const { return Animation(id, *this); }
	SDL_Palette *open_pal(res_id id) const;

	const DrsList operator[](unsigned pos) const noexcept {
		return DrsList(*this, pos);
	}

	constexpr size_t size() const noexcept { return hdr->nlist; }
	constexpr bool empty() const noexcept { return size() == 0; }
};

extern class Assets final {
public:
	std::vector<std::unique_ptr<DRS>> blobs;
	std::vector<std::unique_ptr<Blob>> ttf;

	bool open_item(DrsItem &item, res_id id, DrsType type) const noexcept;
	bool open_item(DrsItem &item, res_id id, DrsType type, unsigned &pos) const noexcept;

	SDL_Palette *open_pal(res_id id) const;
} assets;

}
