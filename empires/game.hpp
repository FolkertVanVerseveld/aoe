/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Core game model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

// TODO use quadtree for units etc.

#pragma once

#include <cstdint>

#include <algorithm>
#include <memory>
#include <string>

#include <map>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>

#include "lang.h"

#include "image.hpp"
#include "render.hpp"
#include "world.hpp"

#define TILE_WIDTH 32
#define TILE_HEIGHT 16

#define MOVE_SPEED 16

#define MAX_PLAYER_COUNT 8

#define CIV_EGYPTIAN 0
#define CIV_GREEK 1
#define CIV_BABYLONIAN 2
#define CIV_ASSYRIAN 3
#define CIV_MINOAN 4
#define CIV_HITTITE 5
#define CIV_PHOENICIAN 6
#define CIV_SUMERIAN 7
#define CIV_PERSIAN 8
#define CIV_SHANG 9
#define CIV_YAMATO 10
#define CIV_CHOSON 11

#define MAX_CIVILIZATION_COUNT (CIV_CHOSON + 1)

extern const unsigned menu_bar_tbl[MAX_CIVILIZATION_COUNT];

class Resources {
public:
	unsigned food;
	unsigned wood;
	unsigned gold;
	unsigned stone;

	Resources(unsigned food=0, unsigned wood=0, unsigned gold=0, unsigned stone=0)
		: food(food), wood(wood), gold(gold), stone(stone) {}

	friend bool operator<(const Resources &lhs, const Resources &rhs) {
		return rhs.food > lhs.food || rhs.wood > lhs.wood
			|| rhs.gold > lhs.gold || rhs.stone > lhs.stone;
	}

	friend bool operator>(const Resources &lhs, const Resources &rhs) { return rhs < lhs; }
	friend bool operator<=(const Resources &lhs, const Resources &rhs) { return !(lhs > rhs); }
	friend bool operator>=(const Resources &lhs, const Resources &rhs) { return !(lhs < rhs); }
	friend bool operator==(const Resources &lhs, const Resources &rhs) {
		return lhs.food == rhs.food && lhs.wood == rhs.wood
			&& lhs.gold == rhs.gold && lhs.stone == rhs.stone;
	}
	friend bool operator!=(const Resources &lhs, const Resources &rhs) { return !(lhs == rhs); }
};

const class Resources res_low_default(200, 200, 0, 150);

class Stats {
public:
	virtual unsigned total_score() const = 0;
};

class StatsMilitary final : public Stats {
public:
	unsigned kills, razings, losses;
	unsigned army_count;

	static unsigned max_army_count;

	StatsMilitary() : kills(0), razings(0), losses(0), army_count(0) {}

	virtual unsigned total_score() const {
		return (unsigned)std::max<int>(
			0, 3 * kills + (3 * razings / 2) - (int)losses
			+ (army_count >= max_army_count ? 25 : 0)
		);
	}
};

class StatsEconomy final : public Stats {
public:
	unsigned gold;
	unsigned villagers;
	unsigned long explored;
	unsigned tributed;

	static unsigned long max_explored;
	static unsigned long explore_count;
	static unsigned max_villagers;

	StatsEconomy() : gold(0), villagers(0), explored(0), tributed(0) {}

	virtual unsigned total_score() const {
		// TODO verify this
		int score = gold / 50;

		// TODO verify this
		if (villagers == max_villagers)
			score += 25;

		// TODO verify this
		if (explored == max_explored)
			score += 25;

		// 1 point per 3 procent explored
		score += (100 * explored / explore_count) / 3;

		return (unsigned)std::max<int>(0, score);
	}
};

class StatsReligion final : public Stats {
public:
	unsigned conversions;
	unsigned ruins, artifacts, temples;

	static unsigned max_conversion;
	static unsigned total_religious_objects;

	StatsReligion() : conversions(0), ruins(0), artifacts(0), temples(0) {}

	virtual unsigned total_score() const {
		unsigned score = 10 * ruins + 10 * artifacts;

		if (total_religious_objects) {
			// XXX may not work with teams?
			if (ruins + artifacts == total_religious_objects)
				// TODO verify this
				score += 50;
		}

		return score;
	}
};

class StatsTechnology final : public Stats {
public:
	unsigned technologies;
	bool bronze_first, iron_first;

	static unsigned max_technologies;

	StatsTechnology()
		: technologies(0)
		, bronze_first(false), iron_first(false) {}

	virtual unsigned total_score() const {
		unsigned score = 2 * technologies + 25 * bronze_first + 25 * iron_first;

		if (technologies == max_technologies)
			score += 25;

		return score;
	}
};

class Summary final {
public:
	StatsMilitary military;
	StatsEconomy economy;
	StatsReligion religion;
	StatsTechnology technology;

	Summary() : military(), economy(), religion(), technology() {}

	unsigned total_score() const {
		return military.total_score() + economy.total_score()
			+ religion.total_score() + technology.total_score();
	}
};

class GameSettings final {
public:
	float gamespeed;
	float music_volume, sound_volume;
	float scrollspeed;

	unsigned screen_mode;
	bool twobutton;
	bool help;
	unsigned path_finding;

	GameSettings()
		: gamespeed(1)
		, music_volume(1), sound_volume(1), scrollspeed(1)
		, screen_mode(0)
		, twobutton(true), help(true)
		, path_finding(0) {}
};

class ImageCache {
	Palette pal;
public:
	std::map<unsigned, AnimationTexture> cache;

	ImageCache();

	const AnimationTexture &get(unsigned id);
};

class Player {
public:
	std::string name;
	unsigned civ;
	bool alive;
	Resources resources;
	Summary summary;
	unsigned color;
	Quadtree units;

	Player(const std::string &name, unsigned civ = 0, unsigned color = 0);

	/**
	 * Initialize default stuff
	 */
	void init_dummy();

	void idle();

	virtual void tick() = 0;
};

class PlayerHuman final : public Player {
public:
	PlayerHuman(const std::string &name = "You", unsigned color = 0);

	virtual void tick() override final;
};

class PlayerComputer final : public Player {
public:
	PlayerComputer(unsigned color = 0);

	virtual void tick() override final;
};

class Game final {
	bool run;
	int x, y, w, h;
	unsigned keys;
	unsigned player_index;
public:
	bool paused;
	Map map;
	std::unique_ptr<ImageCache> cache;
	// XXX use set?
	// just use vector: negleglible delay for using something more sophisticated for just 9 players
	std::vector<std::shared_ptr<Player>> players;
	// units is internal 2d grid, display_units is isometric 2d grid
	Quadtree units;

	std::vector<std::shared_ptr<Unit>> selected;

	RendererState state;

	SDL_Cursor *cursor;

	Game();
	~Game();

	void reset(unsigned players = 2);
	void dispose(void);

	void resize(MapSize size);
	size_t player_count() { return players.size(); }

	void idle();

	void start();
	void stop();

	void set_cursor(unsigned index);

	bool mousedown(SDL_MouseButtonEvent *event);

	bool keydown(SDL_KeyboardEvent *event);
	bool keyup(SDL_KeyboardEvent *event);

	/* Change game viewport */
	void reshape(int x, int y, int w, int h) {
		this->x = x; this->y = y;
		this->w = w; this->h = h;
		map.reshape(x, y, w, h);
	}

	void draw();
	void spawn(Unit *obj);

	const Player *controlling_player() const;
};

extern Game game;
