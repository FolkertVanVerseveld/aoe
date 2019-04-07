/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

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

unsigned Unit::count = 0;

Unit::Unit(
	unsigned hp,
	int x, int y, unsigned size, int w, int h,
	unsigned sprite_index,
	unsigned color,
	int dx, int dy
)
	: hp(hp)
	, pos(x, y), dx(dx), dy(dy), w(w), h(h), size(size)
	, animation(game.cache->get(sprite_index)), image_index(0)
	, color(color)
{
	++count;
}

void Unit::draw(Map &map) const
{
	Point scr;
	pos.to_screen(scr);
	scr.move(dx, dy);
	animation.draw(scr.x, scr.y, image_index, color);
}

void Unit::draw_selection(Map &map) const
{
	Point scr;
	pos.to_screen(scr);
	scr.move(dx, dy);
	animation.draw_selection(scr.x, scr.y, size);
}

Player::Player(const std::string &name, unsigned civ, unsigned color)
	: name(name), civ(civ), alive(true)
	, resources(res_low_default), summary(), color(color), units()
{
	dbgf("init player %s: civ=%u, col=%u\n", name.c_str(), civ, color);
}

Building::Building(
	unsigned id, unsigned p_id,
	int x, int y, unsigned size, int w, int h, unsigned color
)
	: Unit(0, x, y, size, w, h, id, color, 0, 0)
	, overlay(game.cache->get(p_id)) , overlay_index(0)
{
}

void Building::draw(Map &map) const {
	Point scr;

	Unit::draw(map);

	pos.to_screen(scr);
	scr.move(dx, dy);

	overlay.draw(scr.x, scr.y, overlay_index, color);
}

void Player::init_dummy() {
	Map &map = game.map;

	int x = rand() % map.w;
	int y = rand() % map.h;

	switch (color) {
	case 0:
		x = y = 0;
		break;
	case 1:
		x = map.w - 3;
		y = 0;
		break;
	}
	x += 2;
	++y;

	game.spawn(
		new Building(
			DRS_TOWN_CENTER_BASE,
			DRS_TOWN_CENTER_PLAYER,
			x, y, 3 * TILE_WIDTH, color
		)
	);

	game.spawn(
		new Building(
			DRS_BARRACKS_BASE,
			DRS_BARRACKS_PLAYER,
			x, y + 3, 3 * TILE_WIDTH, color
		)
	);

	x = rand() % map.w;
	y = rand() % map.h;

	game.spawn(new Unit(0, x, y, 3, 0, 14, DRS_VILLAGER_STAND, color));
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
	, map(), cache(), players(), state(), cursor(nullptr)
{
}

Game::~Game()
{
}

void Game::dispose(void) {
	if (run)
		panic("Bad game state");

	players.clear();
	cache.reset();

	if (cursor) {
		SDL_SetCursor(SDL_GetDefaultCursor());
		SDL_FreeCursor(cursor);
		cursor = nullptr;
	}
}

void Game::reset(unsigned players) {
	if (run)
		panic("Bad game state");

	player_index = 0;
	units.clear();
	assert(Unit::count == 0);

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

	if (dx || dy)
		dbgf("view: (%d,%d)\n", state.view_x, state.view_y);
}

void Game::start() {
	for (auto p : players)
		p->init_dummy();

	run = true;
	in_game = 1;
	music_index = 0;
	mus_play(MUS_GAME1);

	set_cursor(0);
}

void Game::set_cursor(unsigned index) {
	const AnimationTexture &cursors = cache->get(DRS_CURSORS);
	if (cursor)
		SDL_FreeCursor(cursor);
	cursor = SDL_CreateColorCursor(cursors.images[index].surface.get(), 0, 0);
	if (!cursor)
		panic("Bad cursor");
	SDL_SetCursor(cursor);
}

void Game::stop() {
	run = false;

	if (cursor) {
		SDL_SetCursor(SDL_GetDefaultCursor());
		SDL_FreeCursor(cursor);
		cursor = nullptr;
	}
}

void Game::draw() {
	// draw terrain
	int tleft, ttop, tright, tbottom;
	int ty, tx, th, tw, y, x;
	const AnimationTexture &bkg = cache->get(DRS_TERRAIN_DESERT);
	const uint8_t *data = map.map.get();

	canvas.push_state(state);
	RendererState &s = canvas.get_state();
	s.move_view(-this->x, -this->y);

	/*
	y+ axis is going from left to top corner
	x+ axis is going from top to right corner
	*/

	// TODO compute frustum
	th = map.h; tw = map.w;

	// FIXME properly compute offsets
	int tlo = 2; // -1
	int tbo = 2;
	int tro = -4;

	// TODO compute frustum
	tleft = (state.view_x + tlo * TILE_WIDTH) / TILE_WIDTH;
	tright = tleft + this->w / TILE_WIDTH + tro;
	if (tleft < 0)
		tleft = 0;
	if (tright > tw)
		tright = tw;

	// TODO compute bottom and top frustum
	// dit is lastiger omdat we per rij tekenen...
	tbottom = 0;
	ttop = (-state.view_y + tbo * TILE_HEIGHT) / TILE_HEIGHT;

	if (ttop < 0)
		ttop = 0;
	if (ttop > th)
		ttop = th;

	int ttp = ttop - 1;

	x = tleft * TILE_WIDTH;
	y = tleft * TILE_HEIGHT;

	for (ty = tbottom; ty < ttop; ++ty) {
		int xp = x, yp = y;
		for (tx = tleft; tx < tright; ++tx) {
			bkg.draw(x, y, data[ty * tw + tx]);
			x += TILE_WIDTH;
			y += TILE_HEIGHT;
		}
		x = xp + TILE_WIDTH;
		y = yp - TILE_HEIGHT;
		if (tleft > 0) {
			--tleft;
			x -= TILE_WIDTH;
			y -= TILE_WIDTH / 2;
		}
		if (tleft <= tlo)
			--tright;
	}

	std::vector<std::shared_ptr<Unit>> objects;
	// FIXME compute proper bounds
	AABB bounds(tw, th);
	units.query(objects, bounds);

	// draw all selected objects
	for (auto unit : objects)
		if (true) //if (unit.selected)
			unit->draw_selection(map);

	// determine drawing order: priority = -y
	std::sort(objects.begin(), objects.end(), [](std::shared_ptr<Unit> &lhs, std::shared_ptr<Unit> &rhs) {
		return lhs->pos.y > rhs->pos.y;
	});

	// draw all found objects
	for (auto unit : objects)
		unit->draw(map);

	canvas.pop_state();
}

bool Game::mousedown(SDL_MouseButtonEvent *event) {
	// translate mouse coordinates to in game
	int mx, my;

	mx = event->x + state.view_x;
	my = event->y + state.view_y;

	dbgf("TODO: mousedown (%d,%d) -> (%d,%d)\n", event->x, event->y, mx, my);
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
const Player *Game::controlling_player() const {
	return players[player_index].get();
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
