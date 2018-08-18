/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Core game model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "game.hpp"

#include <cstdio>
#include <random>

#include "../setup/dbg.h"
#include "../setup/def.h"
#include "../setup/res.h"

std::vector<std::shared_ptr<Player>> players;
Map map;

unsigned StatsMilitary::max_army_count = 0;

unsigned long StatsEconomy::max_explored = 0;
unsigned long StatsEconomy::explore_count = 0;
unsigned StatsEconomy::max_villagers = 0;

unsigned StatsReligion::max_conversion = 0;
unsigned StatsReligion::total_religious_objects = 0;

unsigned StatsTechnology::max_technologies = 0;

Unit::Unit(unsigned hp, int x, int y, unsigned w, unsigned h, const AnimationTexture &animation)
	: hp(hp), x(x), y(y), w(w), h(h), animation(animation), image_index(0)
{
}

void Unit::draw(unsigned color) const
{
	animation.draw(x, y, image_index);
}

Player::Player(const std::string &name, unsigned civ, unsigned color)
	: name(name), civ(civ), alive(true)
	, resources(res_low_default), summary(), color(color), units()
{
}

void Player::idle() {
	// TODO do logic
	tick();
}

void Player::draw() const {
	for (auto &x : units)
		x->draw(color);
}

void PlayerHuman::tick() {
}

extern struct pe_lib lib_lang;

std::random_device rd;
unsigned random_civ;

std::string random_name() {
	char buf[256];

	std::mt19937 gen(rd());
	std::uniform_int_distribution<unsigned> civdist(0, MAX_CIVILIZATION_COUNT - 1);
	random_civ = civdist(gen);

	load_string(&lib_lang, STR_CIV_EGYPTIAN + random_civ, buf, sizeof buf);
	//dbgf("random civ: %s\n", buf);

	load_string(&lib_lang, STR_BTN_CIVTBL + 10 * random_civ, buf, sizeof buf);
	unsigned name_count;
	sscanf(buf, "%u", &name_count);
	decltype(civdist) namedist(0, name_count - 1);

	load_string(&lib_lang, STR_BTN_CIVTBL + 10 * random_civ + namedist(gen) + 1, buf, sizeof buf);
	//dbgf("random name: %s\n", buf);
	return std::string(buf);
}

PlayerHuman::PlayerHuman(const std::string &name) : Player(name, 0) {
	random_name();
	civ = random_civ;
}

PlayerComputer::PlayerComputer() : Player(random_name(), 0) {
	civ = random_civ;
}

void PlayerComputer::tick() {
}

Map::Map(unsigned w, unsigned h)
	: map(new uint8_t[w * h])
{
}

void Map::resize(MapSize size)
{
	switch (size) {
	default:
		dbgf("unknown map size: %d\n", size);
	case TINY:
		resize(72, 72);
		break;
	}
}

void Map::resize(unsigned w, unsigned h)
{
	map.reset(new uint8_t[w * h]);
}
