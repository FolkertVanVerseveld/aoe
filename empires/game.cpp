/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Core game model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "game.hpp"

std::vector<std::shared_ptr<Player>> players;

void Player::idle() {
	// TODO do logic
	tick();
}

void PlayerHuman::tick() {
}

void PlayerComputer::tick() {
}
