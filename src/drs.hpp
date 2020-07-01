/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#pragma once

#include "types.hpp"
#include "fs.hpp"

namespace genie {

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
	/* sfx */
	button4 = 50300,
	chat = 50302,
	error = 50303,
	win = 50320,
	lost = 50321,
	game = 5036,
	/* hud */
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
};

class DRS final {
public:
	Blob blob;
	const io::DrsHdr *hdr;

	DRS(const std::string &name, iofd fd, bool map);

	/** Try to load the specified game asset using the specified type and unique identifier */
	bool open_item(io::DrsItem &item, res_id id, DrsType type);
};

}
