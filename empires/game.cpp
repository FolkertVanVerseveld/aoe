/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Core game model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "game.hpp"

#include <cstdio>

#include "../setup/dbg.h"
#include "../setup/def.h"
#include "../setup/res.h"

std::vector<std::shared_ptr<Player>> players;

void Player::idle() {
	// TODO do logic
	tick();
}

void PlayerHuman::tick() {
}

extern struct pe_lib lib_lang;

PlayerComputer::PlayerComputer(const std::string &name, unsigned civ) : Player(name, civ) {
	char buf[256];

	unsigned civ_id = STR_CIV_EGYPTIAN;
	unsigned name_count;

	for (unsigned i = 0, n = MAX_CIVILIZATION_COUNT; i < n; ++i, ++civ_id) {
		load_string(&lib_lang, civ_id, buf, sizeof buf);
		dbgf("civ %2u: %s\n", i, buf);

		unsigned tbl_id = STR_BTN_CIVTBL + 10 * i;

		load_string(&lib_lang, tbl_id, buf, sizeof buf);
		if (!sscanf(buf, "%u", &name_count))
			continue;

		dbgf("names: %u\n", name_count);
		for (unsigned j = tbl_id + 1, k = j + name_count, n = 0; j < k; ++j, ++n) {
			load_string(&lib_lang, j, buf, sizeof buf);
			dbgf("%2u: %s\n", n, buf);
		}
	}
}

void PlayerComputer::tick() {
}
