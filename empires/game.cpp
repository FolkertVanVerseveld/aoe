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
#include "sfx.h"
#include "ui.h"

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
	unsigned sprite_index,
	unsigned color,
	int dx, int dy
)
	: hp(hp)
	, bounds(Point(x, y), Point(w, h)), dx(dx), dy(dy)
	, animation(game.cache->get(sprite_index)), image_index(0)
	, color(color)
{
}

void Unit::draw(Map &map) const
{
	Point scr;

	bounds.pos.to_screen(scr);
	scr.move(dx, dy);

	animation.draw(scr.x, scr.y, image_index, color);
}

Player::Player(const std::string &name, unsigned civ, unsigned color)
	: name(name), civ(civ), alive(true)
	, resources(res_low_default), summary(), color(color), units()
{
}

Building::Building(
	unsigned id, unsigned p_id,
	int x, int y, unsigned w, unsigned h, unsigned color
)
	: Unit(0, x, y, w, h, id, color, TILE_WIDTH / 2, TILE_HEIGHT / 2)
	, overlay(game.cache->get(p_id)) , overlay_index(0)
{
}

void Building::draw(Map &map) const {
	Point scr;

	Unit::draw(map);

	bounds.pos.to_screen(scr);
	scr.move(dx, dy);

	overlay.draw(scr.x, scr.y, overlay_index, color);
}

void Player::init_dummy() {
	Map &map = game.map;

	int x = rand() % map.w;
	int y = rand() % map.h;

	game.spawn(
		new Building(
			DRS_TOWN_CENTER_BASE,
			DRS_TOWN_CENTER_PLAYER,
			x, y, 3, 3, color
		)
	);

	x = rand() % map.w;
	y = rand() % map.h;

	game.spawn(new Unit(0, x, y, 1, 1, DRS_VILLAGER_STAND, color));
}

void Player::idle() {
	// TODO do logic
	tick();
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

PlayerHuman::PlayerHuman(const std::string &name, unsigned color) : Player(name, 0, color) {
	random_name();
	civ = random_civ;
}

PlayerComputer::PlayerComputer(unsigned color) : Player(random_name(), 0, color) {
	civ = random_civ;
}

void PlayerComputer::tick() {
	dbgf("stub: %s\n", __func__);
}

Map::Map(unsigned w, unsigned h)
	: map(new uint8_t[w * h]), heightmap(new uint8_t[w * h])
{
}

void Map::resize(MapSize size)
{
	switch (size) {
	default:
		dbgf("unknown map size: %d\n", size);
	case TINY : resize(72); break;
	case MICRO: resize(24); break;
	}
}

void Map::resize(unsigned w, unsigned h)
{
	map.reset(new uint8_t[w * h]);
	heightmap.reset(new uint8_t[w * h]);

	this->w = w;
	this->h = h;

	// dummy init
	uint8_t *data = map.get(), *hdata = heightmap.get();

	for (unsigned y = 0; y < h; ++ y)
		for (unsigned x = 0; x < w; ++x) {
			data[y * w + x] = rand() % 9;
			hdata[y * w + x] = 0;
		}
}

void Map::reshape(int view_x, int view_y, int view_w, int view_h)
{
	// TODO check how top and bottom boundary are determined in original
	Point tile, scr;
	int d = 12;

	tile.x = tile.y = 0;
	tile.to_screen(scr);
	left = view_x + scr.x - d * TILE_WIDTH;
	tile.x = w - 1;
	tile.to_screen(scr);
	bottom = view_y + scr.y - (view_h - d * TILE_HEIGHT);
	tile.y = h - 1;
	tile.to_screen(scr);
	right = view_x + scr.x - (view_w - d * TILE_WIDTH);
	tile.x = 0;
	tile.to_screen(scr);
	top = view_y + scr.y - d * TILE_HEIGHT;
}

#define KEY_LEFT 1
#define KEY_RIGHT 2
#define KEY_UP 4
#define KEY_DOWN 8

Game::Game()
	: run(false), x(0), y(0), w(800), h(600)
	, keys(0), paused(false)
	, map(), cache(), players(), state()
{
}

void Game::dispose(void) {
	if (run)
		panic("Bad game state");

	players.clear();
	cache.reset();
}

void Game::reset(unsigned players) {
	if (run)
		panic("Bad game state");

	cache.reset(new ImageCache());
	resize(MICRO);

	this->players.clear();

	unsigned color = 0;

	if (players) {
		this->players.emplace_back(new PlayerHuman("you", color++));

		while (--players)
			this->players.emplace_back(new PlayerComputer(color++));
	}

	stats_reset();
}

void Game::resize(MapSize size) {
	map.resize(size);
}

void Game::idle() {
	if (paused)
		return;

	// Always repaint the screen
	canvas_dirty();

	// Determine displacement vector
	int dx = 0, dy = 0;

	if (keys & KEY_DOWN)
		dy += MOVE_SPEED;
	if (keys & KEY_UP)
		dy -= MOVE_SPEED;
	if (keys & KEY_RIGHT)
		dx += MOVE_SPEED;
	if (keys & KEY_LEFT)
		dx -= MOVE_SPEED;

	state.move_view(dx, dy);

	// limit viewport bounds
	// TODO strafe viewport if hitting boundaries
	int vx = state.view_x, vy = state.view_y;
	if (vx < map.left)
		vx = map.left;
	else if (vx > map.right)
		vx = map.right;
	if (vy < map.top)
		vy = map.top;
	else if (vy > map.bottom)
		vy = map.bottom;

	state.set_view(vx, vy);
}

void Game::start() {
	for (auto p : players)
		p->init_dummy();

	run = true;
	in_game = 1;
	music_index = 0;
	mus_play(MUS_GAME1);
}

void Game::stop() {
	run = false;
}

void Game::draw() {
	// draw terrain
	int ty, tx, th, tw, y, x;
	const AnimationTexture &bkg = cache->get(DRS_TERRAIN_DESERT);
	const uint8_t *data = map.map.get();

	canvas.push_state(state);

	/*
	y+ axis is going from left to top corner
	x+ axis is going from top to right corner
	*/

	th = map.h; tw = map.w;

	y = th * TILE_HEIGHT / 2;
	x = y = 0;

	for (ty = 0; ty < th; ++ty) {
		int xp = x, yp = y;
		for (tx = 0; tx < tw; ++tx) {
			bkg.draw(
				x, y, data[ty * tw + tx]
			);
			x += TILE_WIDTH;
			y += TILE_HEIGHT;
		}
		x = xp + TILE_WIDTH;
		y = yp - TILE_HEIGHT;
	}

	std::vector<std::weak_ptr<Unit>> objects;
	// FIXME compute proper bounds
	AABB bounds(tw, th);
	units.query(objects, bounds);

	// FIXME sort objects in proper drawing order
	// just use: priority = -y

	// draw all found objects
	for (auto &o : objects)
		if (auto unit = o.lock())
			unit->draw(map);

	canvas.pop_state();
}

bool Game::keydown(SDL_KeyboardEvent *event) {
	unsigned virt = event->keysym.sym;

	if (paused) {
		switch (virt) {
		case SDLK_F3:
			break;
		default:
			return false;
		}

		return true;
	}

	switch (virt) {
	case SDLK_DOWN : keys |= KEY_DOWN ; break;
	case SDLK_UP   : keys |= KEY_UP   ; break;
	case SDLK_RIGHT: keys |= KEY_RIGHT; break;
	case SDLK_LEFT : keys |= KEY_LEFT ; break;
	case ' ':
		break;
	default:
		return false;
	}

	canvas_dirty();
	return true;
}

bool Game::keyup(SDL_KeyboardEvent *event) {
	unsigned virt = event->keysym.sym;

	if (paused) {
		switch (virt) {
		case SDLK_F3:
			goto pause;
		default:
			return false;
		}

		return true;
	}

	switch (virt) {
	case SDLK_DOWN : keys &= ~KEY_DOWN ; break;
	case SDLK_UP   : keys &= ~KEY_UP   ; break;
	case SDLK_RIGHT: keys &= ~KEY_RIGHT; break;
	case SDLK_LEFT : keys &= ~KEY_LEFT ; break;
	case SDLK_F3:
pause:
		paused = !paused;

		if (paused)
			keys = 0;

		canvas_dirty();
		break;
	default: return false;
	}

	return true;
}

// FIXME get real controlling player
const Player *Game::get_controlling_player() const {
	return players[0].get();
}

void Game::spawn(Unit *obj) {
	std::shared_ptr<Unit> unit = std::shared_ptr<Unit>(obj);

	if (!units.put(unit))
		panic("Game: could not spawn unit");
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
