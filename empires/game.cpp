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
#include "lang.h"

#include "../setup/dbg.h"
#include "../setup/def.h"
#include "../setup/res.h"

Game game;
extern struct pe_lib lib_lang;

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
unsigned Unit::id_counter = 0;

Unit::Unit(
	unsigned hp,
	int x, int y, unsigned size, int w, int h,
	unsigned sprite_index,
	unsigned color,
	int dx, int dy
)
	: hp(hp), hp_max(hp)
	, pos(x, y), dx(dx), dy(dy), w(w), h(h), size(size)
	, animation(game.cache->get(sprite_index)), image_index(0)
	, color(color)
{
	++count;
	id = ++id_counter;
}

static void dump_aabb(const AABB &b)
{
	printf("[(%d,%d), (%d,%d)]\n", b.pos.x - b.hbounds.x, b.pos.y - b.hbounds.y,
		b.pos.x + b.hbounds.x, b.pos.y + b.hbounds.y);
}

void Unit::draw(Map &map) const
{
	Point scr;
	const uint8_t *hdata = map.heightmap.get();
	pos.to_screen(scr);
	scr.move(dx, dy - TILE_HEIGHT * hdata[pos.y * map.w + pos.x]);
	animation.draw(scr.x, scr.y, image_index, color);
}

void Unit::draw_selection(Map &map) const
{
	Point scr;
	const uint8_t *hdata = map.heightmap.get();
	pos.to_screen(scr);
	scr.move(dx, dy - TILE_HEIGHT * hdata[pos.y * map.w + pos.x]);
	animation.draw_selection(scr.x, scr.y, size);
}

void Unit::to_screen(Map &map)
{
	Point spos;
	const uint8_t *hdata = map.heightmap.get();
	Image &img = animation.images[image_index];
	SDL_Surface *surf = img.surface.get();

	pos.to_screen(spos);
	spos.move(dx, dy - TILE_HEIGHT * hdata[pos.y * map.w + pos.x]);

	//dbgf("%d,%d,%d\n", pos.x, img.hotspot_x, surf->w / 2);
	//dbgf("%d,%d,%d\n", pos.y, img.hotspot_y, surf->h / 2);
	scr.pos.x = spos.x - img.hotspot_x + surf->w / 2;
	scr.pos.y = spos.y - img.hotspot_y + surf->h / 2;
	scr.hbounds.x = surf->w / 2;
	scr.hbounds.y = surf->h / 2;

	//canvas.draw_rect(bounds.pos.x - bounds.hbounds.x, bounds.pos.y - bounds.hbounds.y, bounds.pos.x + bounds.hbounds.x, bounds.pos.y + bounds.hbounds.y);
}

void Building::to_screen(Map &map)
{
	Point spos;
	const uint8_t *hdata = map.heightmap.get();
	Image &img = animation.images[image_index];
	SDL_Surface *surf = img.surface.get();

	if (img.hotspot_x > surf->w || img.hotspot_y > surf->h ||
		img.hotspot_x < -surf->w || img.hotspot_y < -surf->h) {
		// probably bogus hotspot, use overlay
		img = overlay.images[image_index];
		surf = img.surface.get();
	}

	pos.to_screen(spos);
	spos.move(dx, dy - TILE_HEIGHT * hdata[pos.y * map.w + pos.x]);

	//dbgf("%d,%d,%d\n", pos.x, img.hotspot_x, surf->w / 2);
	//dbgf("%d,%d,%d\n", pos.y, img.hotspot_y, surf->h / 2);
	scr.pos.x = spos.x - img.hotspot_x + surf->w / 2;
	scr.pos.y = spos.y - img.hotspot_y + surf->h / 2;
	scr.hbounds.x = surf->w / 2;
	scr.hbounds.y = surf->h / 2;

	//canvas.draw_rect(bounds.pos.x - bounds.hbounds.x, bounds.pos.y - bounds.hbounds.y, bounds.pos.x + bounds.hbounds.x, bounds.pos.y + bounds.hbounds.y);
}

Player::Player(const std::string &name, unsigned civ, unsigned color)
	: name(name), civ(civ), alive(true)
	, resources(res_low_default), summary(), color(color), units()
{
	dbgf("init player %s: civ=%u, col=%u\n", name.c_str(), civ, color);
}

Building::Building(
	unsigned hp, unsigned id, unsigned p_id,
	int x, int y, unsigned size, int w, int h, unsigned color
)
	: Unit(hp, x, y, size, w, h, id, color, 0, 0)
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

static void spawn_town_center(int x, int y, unsigned color)
{
	game.spawn(
		new Building(
			600,
			DRS_TOWN_CENTER_BASE,
			DRS_TOWN_CENTER_PLAYER,
			x, y, 3 * TILE_WIDTH, color
		)
	);
}

static void spawn_barracks(int x, int y, unsigned color)
{
	game.spawn(
		new Building(
			350,
			DRS_BARRACKS_BASE,
			DRS_BARRACKS_PLAYER,
			x, y, 3 * TILE_WIDTH, color
		)
	);
}

static void spawn_academy(int x, int y, unsigned color)
{
	game.spawn(
		new Building(
			350,
			DRS_ACADEMY_BASE,
			DRS_ACADEMY_PLAYER,
			x, y, 3 * TILE_WIDTH, color
		)
	);
}

static void spawn_villager(int x, int y, unsigned color)
{
	game.spawn(new Villager(25, x, y, color));
}

static void spawn_berry_bush(int x, int y)
{
	game.spawn(new StaticResource(
		x, y, 24, 0, 0, DRS_BERRY_BUSH, SR_FOOD, 150
	));
}

static void spawn_desert_tree(int x, int y)
{
	game.spawn(new StaticResource(
		x, y, TILE_WIDTH, 0, 0, DRS_DESERT_TREE1 + rand() % 4, SR_WOOD, 40
	));
}

static void spawn_gold(int x, int y)
{
	game.spawn(new StaticResource(x, y, 40, 0, 0, DRS_GOLD, SR_GOLD, 400));
}

static void spawn_stone(int x, int y)
{
	game.spawn(new StaticResource(x, y, 40, 0, 0, DRS_STONE, SR_STONE, 250));
}

void DynamicUnit::move(int tx, int ty, int dx, int dy)
{
	target.x = tx;
	target.y = ty;
	tdx = dx;
	tdy = dy;
}

void DynamicUnit::tick()
{
}

void Player::init_dummy() {
	Map &map = game.map;

	int x = rand() % map.w;
	int y = rand() % map.h;

	switch (color) {
	case 0:
		x = y = 1;
		break;
	case 1:
		x = map.w - 2;
		y = map.h - 8;
		break;
	}

	spawn_town_center(x, y, color);
	spawn_academy(x, y + 3, color);
	spawn_barracks(x, y + 6, color);

	// 3 villagers
	for (int i = 0; i < 3; ++i)
		spawn_villager(rand() % map.w, rand() % map.h, color);

	// 8 bushes
	for (int i = 0; i < 8; ++i)
		spawn_berry_bush(rand() % map.w, rand() % map.h);

	// 6 piles of stone and gold
	for (int i = 0; i < 6; ++i) {
		spawn_gold(rand() % map.w, rand() % map.h);
		spawn_stone(rand() % map.w, rand() % map.h);
	}

	// 24 trees
	for (int i = 0; i < 24; ++i)
		spawn_desert_tree(rand() % map.w, rand() % map.h);

#if 0
	for (int i = 0; i < map.w; ++i)
		spawn_desert_tree(i, 0);
#endif
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

	selected.clear();
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
	selected.clear();
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

	// Dynamic objects game tick
	for (auto &unit : units.dynamic_objects)
		unit->tick();

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

#if 0
	if (dx || dy)
		dbgf("view: (%d,%d)\n", state.view_x, state.view_y);
#endif
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
	const uint8_t *data = map.map.get(), *hdata = map.heightmap.get();

	canvas.push_state(state);
	RendererState &s = canvas.get_state();
	s.move_view(-this->x, -this->y);

	/*
	y+ axis is going from left to top corner
	x+ axis is going from top to right corner
	*/

	th = map.h; tw = map.w;

	// FIXME properly compute offsets
	int tlo = -1; // -1
	int tto = -2;
	int tro = 1;
	// TODO compensate with heightmap
	int tbo = 0 + 0; // TODO /+ 0/max(height_map)/

	// compute horizontal frustum
	tleft = (state.view_x + tlo * TILE_WIDTH) / TILE_WIDTH;
	if (tleft < 0)
		tleft = 0;
	tright = tw;

	// TODO compute vertical frustum
	// TODO compute bottom and top frustum
	// dit is lastiger omdat we zowel de x als y positie van de view mee moeten nemen
	// TODO bepaal gewoon de maximale rij aan de rechterkant
	// en zorg dat tleft zich aanpast wanneer de linkerkant boven het scherm uitgaat
	ttop = th;

	int ttp = ttop - 1;
	int count = 0;

	x = tleft * TILE_WIDTH;
	y = tleft * TILE_HEIGHT;

	// XXX analyse if and how we are going to optimize this
	tbottom = 0;

	for (ty = tbottom; ty < ttop; ++ty) {
		int xp = x, yp = y;
		tright = tleft + ((int)this->w + tro * TILE_WIDTH - (x - state.view_x)) / TILE_WIDTH;
		if (tright > tw)
			tright = tw;
		for (tx = tleft; tx < tright; ++tx) {
			if (y >= state.view_y + this->h + tbo * TILE_HEIGHT)
				break;
			bkg.draw(x, y - TILE_HEIGHT * hdata[ty * tw + tx], data[ty * tw + tx]);
			x += TILE_WIDTH;
			y += TILE_HEIGHT;
		}
		x = xp + TILE_WIDTH;
		y = yp - TILE_HEIGHT;
		if (y < state.view_y + tto * TILE_HEIGHT) {
			++count;
			++tleft;
			x += TILE_WIDTH;
			if (x >= state.view_x + this->w + tro * TILE_WIDTH)
				break;
			y += TILE_HEIGHT;
		} else if (tleft > 0) {
			--tleft;
			x -= TILE_WIDTH;
			y -= TILE_HEIGHT;
		}
	}

	std::vector<std::shared_ptr<Unit>> objects;
	// FIXME compute proper bounds
	AABB bounds(tw, th);
	units.query(objects, bounds);

	// draw all selected objects
	for (auto unit : selected)
		unit->draw_selection(map);

	for (auto unit : objects)
		unit->to_screen(map);

	// determine drawing order: priority = -y
	std::sort(objects.begin(), objects.end(), [](std::shared_ptr<Unit> &lhs, std::shared_ptr<Unit> &rhs) {
		return lhs->scr.pos.y < rhs->scr.pos.y;
	});

	// draw all found objects
	for (auto unit : objects)
		unit->draw(map);

	canvas.pop_state();
}

static unsigned unit_name_id(unsigned id)
{
	switch (id) {
	case DRS_VILLAGER_STAND  : return STR_UNIT_VILLAGER;
	case DRS_TOWN_CENTER_BASE: return STR_BUILDING_TOWN_CENTER;
	case DRS_BARRACKS_BASE   : return STR_BUILDING_BARRACKS;
	case DRS_ACADEMY_BASE    : return STR_BUILDING_ACADEMY;
	case DRS_BERRY_BUSH      : return STR_UNIT_BERRY_BUSH;
	case DRS_DESERT_TREE1    : return STR_UNIT_DESERT_TREE1;
	case DRS_DESERT_TREE2    : return STR_UNIT_DESERT_TREE2;
	case DRS_DESERT_TREE3    : return STR_UNIT_DESERT_TREE3;
	case DRS_DESERT_TREE4    : return STR_UNIT_DESERT_TREE4;
	case DRS_GOLD            : return STR_UNIT_GOLD;
	case DRS_STONE           : return STR_UNIT_STONE;
	default:
		dbgf("%s: bad id: %u\n", __func__, id);
		break;
	}
	return STR_UNIT_UNKNOWN;
}

void Game::draw_hud(unsigned w, unsigned h) {
	if (selected.size() == 0)
		return;

	Unit *u = selected.begin()->get();
	Building *b = dynamic_cast<Building*>(u);
	StaticResource *res = dynamic_cast<StaticResource*>(u);

	unsigned drs = u->drs_id(), icon_id, icon_img;

	canvas.col(0);
	canvas.fill_rect(5, 481, 132, 595);

	// draw icon
	icon_id = DRS_BUILDINGS1;
	icon_img = 0;

	switch (drs) {
	case DRS_BARRACKS_BASE   : icon_img = ICON_BARRACKS1; break;
	case DRS_TOWN_CENTER_BASE: icon_img = ICON_TOWN_CENTER1; break;
	case DRS_ACADEMY_BASE    : icon_img = ICON_ACADEMY   ; break;
	default:
		icon_id = 0;
		break;
	}

	// no matching building found, try units
	if (!icon_id) {
		icon_id = DRS_UNITS;

		switch (drs) {
		case DRS_VILLAGER_STAND: icon_img = ICON_VILLAGER  ; break;
		case DRS_BERRY_BUSH    : icon_img = ICON_BERRY_BUSH; break;
		case DRS_DESERT_TREE1  : icon_img = ICON_TREES     ; break;
		case DRS_DESERT_TREE2  : icon_img = ICON_TREES     ; break;
		case DRS_DESERT_TREE3  : icon_img = ICON_TREES     ; break;
		case DRS_DESERT_TREE4  : icon_img = ICON_TREES     ; break;
		case DRS_GOLD          : icon_img = ICON_GOLD      ; break;
		case DRS_STONE         : icon_img = ICON_STONE     ; break;
		default:
			icon_id = 0;
			dbgf("%s: bad hud id: %u\n", __func__, drs);
			break;
		}
	}

	if (icon_id) {
		const AnimationTexture &icons = cache->get(icon_id);
		icons.draw(8, 512, icon_img, u->color);

		if (u->hp_max) {
			const AnimationTexture &bar = cache->get(DRS_HEALTHBAR);
			unsigned img = 25 - 25 * u->hp / u->hp_max;

			if (img > 25)
				img = 25;

			bar.draw(8, 565, img);

			char buf[32];
			snprintf(buf, sizeof buf, "%u/%u", u->hp, u->hp_max);

			canvas.draw_text(8, 577, buf);
		}

		if (u->color < 8) {
			char civbuf[64];
			load_string(&lib_lang, STR_CIV_EGYPTIAN + players[u->color]->civ, civbuf, sizeof civbuf);

			canvas.draw_text(8, 482, civbuf);
		}

		canvas.draw_text(8, 497, unit_name_id(drs));
	}

	// draw unit specific stuff
	const AnimationTexture &stats = cache->get(DRS_STATS);
	const AnimationTexture &tasks = cache->get(DRS_TASKS);

	if (res) {
		unsigned type;
		char buf[16];

		snprintf(buf, sizeof buf, "%u", res->amount);

		switch (res->type) {
		case SR_FOOD: type = ICON_STAT_FOOD; break;
		case SR_WOOD: type = ICON_STAT_WOOD; break;
		case SR_GOLD: type = ICON_STAT_GOLD; break;
		default: type = ICON_STAT_STONE; break;
		}

		stats.draw(58, 512, type);
		canvas.draw_text(94, 516, buf);
	} else if (b) {
		// TODO draw building
	} else {
		// TODO determine unit type
		// just assume it is a villager
		tasks.draw(138, 484, ICON_BUILD);
		tasks.draw(192, 484, ICON_REPAIR);
		tasks.draw(246, 484, ICON_STOP);
	}
	tasks.draw(408, 542, ICON_UNSELECT);
}

static void play_unit_select(unsigned id)
{
	switch (id) {
	case 0:
	case DRS_BERRY_BUSH:
	case DRS_DESERT_TREE1:
	case DRS_DESERT_TREE2:
	case DRS_DESERT_TREE3:
	case DRS_DESERT_TREE4:
	case DRS_GOLD:
	case DRS_STONE:
		break;
	case DRS_BARRACKS_BASE   : sfx_play(SFX_BARRACKS   ); break;
	case DRS_TOWN_CENTER_BASE: sfx_play(SFX_TOWN_CENTER); break;
	case DRS_ACADEMY_BASE    : sfx_play(SFX_ACADEMY    ); break;
	case DRS_VILLAGER_STAND:
		switch (rand() % 5) {
		case 0: sfx_play(SFX_VILLAGER1); break;
		case 1: sfx_play(SFX_VILLAGER2); break;
		case 2: sfx_play(SFX_VILLAGER3); break;
		case 3: sfx_play(SFX_VILLAGER4); break;
		case 4: sfx_play(SFX_VILLAGER5); break;
		}
		break;
	default:
		dbgf("invalid id: %u\n", id);
		break;
	}
}

void Game::button_activate(unsigned id) {
	switch (id) {
	case 18:
		selected.clear();
		break;
	default:
		dbgf("invalid id: %u\n", id);
		break;
	}
}

/*
hud mask rules

army and villager selected

build stop
group ungroup

army selected

standground stop
group       ungroup

fishing ships

stop
group ungroup

light transport (loaded)
unload stop

light transport (unloaded)
stop

when units start moving
the stop icon is added
e.g.: fishing boat does not have any icons
until we let it move or have multiple units selected (i.e. only group/ungroup is shown)
*/

unsigned Game::hud_mask() const {
	if (!selected.size())
		return 0;

	unsigned mask = 0x807;
	Unit *u = (*selected.begin()).get();
	StaticResource *res;

	if (selected.size() > 1)
		mask |= 0x0C0;
	else if (res = dynamic_cast<StaticResource*>(u))
		mask = 0x800;

	return mask;
}

static bool point_in_diamond(Map &map, const Unit *u, int px, int py)
{
	Point scr;
	const uint8_t *hdata = map.heightmap.get();
	u->pos.to_screen(scr);
	scr.move(u->dx, u->dy - TILE_HEIGHT * hdata[u->pos.y * map.w + u->pos.x]);

	int dx, dy, dw, dh;

	dx = px - scr.x;
	dy = py - scr.y;

	if (dx < 0)
		dx = -dx;
	if (dy < 0)
		dy = -dy;

	dw = u->size;
	dh = dw * TILE_HEIGHT / TILE_WIDTH;

	int ymax = (dw - dx) * dh / dw;
	//dbgf("px,py: %d,%d\tdx,dy: %d,%d\tymax: %d\n", px, py, dx, dy, ymax);
	return dy <= ymax;
}

static bool point_to_tile(Map &map, int &tx, int &ty, int &dx, int &dy, int px, int py, bool clip=true)
{
	// determine horizontal screen position.
	// this is easy because horizontal view does not get distorted.
	ty = tx = px / (2 * TILE_WIDTH);
	if (tx < 0 || tx >= (int)map.w || ty < 0 || ty >= (int)map.h) {
		if (!clip)
			return false;
		if (tx < 0)
			tx = 0;
		if (tx >= (int)map.w)
			tx = (int)map.w - 1;
		if (ty < 0)
			ty = 0;
		if (ty >= (int)map.h)
			ty = (int)map.h - 1;
	}

	int scr_x, scr_y, ex, ey;

	scr_x = (tx + ty) * TILE_WIDTH;
	scr_y = (-ty + tx) * TILE_HEIGHT;
	ex = scr_x - px;
	ey = scr_y - py;

	// roughly estimate vertical screen position by converting the horizontal screen position and computing the euclidean distance.
	// this is tricky because this is non-trivial and we need a heightmap.
	//dbgf("tile: (%d,%d), error: (%d,%d)\n", tx, ty, ex, ey);

	ty += ey / (2 * TILE_HEIGHT);
	tx -= ey / (2 * TILE_HEIGHT);

	if (tx < 0 || tx >= (int)map.w || ty < 0 || ty >= (int)map.h) {
		if (!clip)
			return false;
		if (tx < 0)
			tx = 0;
		if (tx >= (int)map.w)
			tx = (int)map.w - 1;
		if (ty < 0)
			ty = 0;
		if (ty >= (int)map.h)
			ty = (int)map.h - 1;
	}

	scr_x = (tx + ty) * TILE_WIDTH;
	scr_y = (-ty + tx) * TILE_HEIGHT;
	ex = scr_x - px;
	ey = scr_y - py;

	dbgf("tile: (%d,%d), error: (%d,%d)\n", tx, ty, ex, ey);

	dx = -ex - TILE_WIDTH;
	dy = -ey - TILE_HEIGHT;
	return true;
}

bool Game::mousedown(SDL_MouseButtonEvent *event) {
	// translate mouse coordinates to in game
	int mx, my;

	mx = event->x + state.view_x;
	my = event->y + state.view_y;

	Point mpos(mx, my);
	std::vector<std::shared_ptr<Unit>> objects;

	switch (event->button) {
	case SDL_BUTTON_LEFT:
		selected.clear();

		// FIXME optimize this
		for (auto unit : units.objects) {
			unit->to_screen(map);

			if (unit->scr.contains(mpos))
				objects.push_back(unit);
		}

		std::sort(objects.begin(), objects.end(), [](std::shared_ptr<Unit> &lhs, std::shared_ptr<Unit> &rhs) {
			return lhs->pos.y < rhs->pos.y;
		});

		// TODO determine if we are selecting a group or a single unit
		for (auto unit : objects) {
			// special handle buildings
			Building *b = dynamic_cast<Building*>(unit.get());
			if (b && !point_in_diamond(map, unit.get(), mx, my)) {
				// TODO add pixel perfect collision detection
				dbgs("skip building");
				continue;
			}

			selected.insert(unit);
			play_unit_select(unit->drs_id());
			break;
		}

		//dbgf("candidates: %d\n", selected.size());
		return true;
	case SDL_BUTTON_RIGHT:
		// convert mouse to tile coordinates
		int tx, ty, dx, dy;
		bool has_villager;

		has_villager = false;

		if (!selected.size() || !point_to_tile(map, tx, ty, dx, dy, mx, my))
			return false;

		//dbgf("TODO: move (%d,%d) -> (%d,%d)\n", mx, my, tx, ty);
		for (auto unit : selected) {
			DynamicUnit *d = dynamic_cast<DynamicUnit*>(unit.get());
			if (0) {
				d->move(tx, ty, dx, dy);
			} else {
				unit->pos.x = tx;
				unit->pos.y = ty;
				unit->dx = dx;
				unit->dy = dy;
			}
			if (dynamic_cast<Villager*>(unit.get()))
				has_villager = true;
			units.update(unit.get());
		}

		if (has_villager)
			sfx_play(SFX_VILLAGER_MOVE);
		break;
	default:
		dbgf("TODO: mousedown (%d,%d) -> (%d,%d)\n", event->x, event->y, mx, my);
		break;
	}

	return false;
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

const Player *Game::controlling_player() const {
	return players[player_index].get();
}

void Game::spawn(Unit *obj) {
	if (!units.put(obj))
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
