/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

namespace genie {

typedef uint16_t res_id; /**< symbolic alias for win32 rc resource stuff */

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
};

}