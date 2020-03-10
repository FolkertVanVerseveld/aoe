/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "types.hpp"

namespace genie {

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

/** All controllable buildings. Note that we some civilizations use different icons. */
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

}