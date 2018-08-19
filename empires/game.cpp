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

#include "drs.h"

#include "../setup/dbg.h"
#include "../setup/def.h"
#include "../setup/res.h"

Game game;

unsigned StatsMilitary::max_army_count = 0;

unsigned long StatsEconomy::max_explored = 0;
unsigned long StatsEconomy::explore_count = 0;
unsigned StatsEconomy::max_villagers = 0;

unsigned StatsReligion::max_conversion = 0;
unsigned StatsReligion::total_religious_objects = 0;

unsigned StatsTechnology::max_technologies = 0;

/** Reset statistics. */
void stats_reset() {
	StatsMilitary::max_army_count = 0;

	StatsEconomy::max_explored = 0;
	StatsEconomy::explore_count = 0;
	StatsEconomy::max_villagers = 0;

	StatsReligion::max_conversion = 0;
	StatsReligion::total_religious_objects = 0;

	StatsTechnology::max_technologies = 0;
}

Unit::Unit(
	unsigned hp,
	int x, int y, unsigned w, unsigned h,
	unsigned sprite_index
)
	: hp(hp)
	, x(x), y(y), w(w), h(h)
	, sprite_index(sprite_index)
	, image_index(0)
{
}

void Unit::draw(unsigned color) const
{
	game.cache->get(sprite_index).draw(x, y, image_index);
}

Player::Player(const std::string &name, unsigned civ, unsigned color)
	: name(name), civ(civ), alive(true)
	, resources(res_low_default), summary(), color(color), units()
{
}

Building::Building(unsigned id, int x, int y)
	: Unit(0, x, y, 1, 1, id) {}

void Player::init_dummy(Map &map) {
	int x, y;
	x = 200 + rand() % 300;
	y = 100 + rand() % 200;

	units.emplace_back(new Building(DRS_TOWN_CENTER_BASE, x, y));
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
	dbgf("stub: %s\n", __func__);
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
	dbgf("stub: %s\n", __func__);
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

Game::Game()
	: run(false), map(), cache(), players()
	, x(0), y(0), w(640), h(480)
{
}

void Game::reset(unsigned players) {
	if (run)
		panic("Bad game state");

	cache.reset(new ImageCache());
	resize(TINY);

	this->players.clear();

	if (players) {
		this->players.emplace_back(new PlayerHuman("you"));

		while (--players)
			this->players.emplace_back(new PlayerComputer());
	}

	stats_reset();
}

void Game::resize(MapSize size) {
	map.resize(size);
}

void Game::start() {
	for (auto p : players)
		p->init_dummy(map);

	run = true;
}

void Game::stop() {
	run = false;
}

void Game::draw() {
	// draw terrain
	int y, x, tile_w = 32, tile_h = 16;

	const AnimationTexture &bkg = cache->get(DRS_TERRAIN_DESERT);

	for (y = this->y - h; y < h; y += tile_h * 2) {
		for (x = this->x - w; x < w; x += tile_w * 2) {
			bkg.draw(x, y, 0);
			bkg.draw(x + tile_w, y + tile_h, 0);
		}
	}

	for (auto p : players)
		p->draw();
}

/* Image cache stuff */

ImageCache::ImageCache() : pal(DRS_MAIN_PALETTE), cache() {}

const AnimationTexture &ImageCache::get(unsigned id)
{
	auto search = cache.find(id);
	if (search != cache.end())
		return search->second;

	cache.emplace(id, AnimationTexture(&pal, id));
	search = cache.find(id);
	if (search != cache.end())
		return search->second;

	panicf("Bad id: %u\n", id);
}
