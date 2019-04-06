/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Virtual world logic model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#pragma once

#include "image.hpp"

#include <vector>

struct Point final {
	int x, y;

	Point() : x(0), y(0) {}
	Point(int x, int y) : x(x), y(y) {}

	friend bool operator==(const Point &lhs, const Point &rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	void to_screen(Point &dst) const;
	/** Try to convert to map coordinates. Returns false if out of bounds. */
	bool to_map(Point &dst) const;

	void move(int dx, int dy) { x += dx; y += dy; }
};

struct AABB final {
	Point pos, hbounds;

	AABB(Point pos = {}, Point hbounds = Point(1, 1))
		: pos(pos), hbounds(hbounds) {}

	AABB(int size) : AABB(size, size) {}
	AABB(int w, int h) : AABB({0, 0}, {w, h}) {}

	bool contains(const Point &p) const {
		return p.x < pos.x + hbounds.x && p.x >= pos.x - hbounds.x
			&& p.y < pos.y + hbounds.y && p.y >= pos.y - hbounds.y;
	}

	bool intersects(const AABB &other) const {
		return (pos.x + hbounds.x >= other.pos.x - other.hbounds.x || pos.x - hbounds.x < other.pos.x + other.hbounds.x)
			&& (pos.y + hbounds.y >= other.pos.y - other.hbounds.y || pos.y - hbounds.y < other.pos.y + other.hbounds.y);
	}

	friend bool operator==(const AABB &lhs, const AABB &rhs) {
		return lhs.pos == rhs.pos && lhs.hbounds == rhs.hbounds;
	}
};

enum MapSize {
	MICRO, // for debug purposes...
	TINY,
	SMALL,
	MEDIUM,
	LARGE,
	HUGE_, // HUGE is already being used somewhere...
};

class Map final {
public:
	std::unique_ptr<uint8_t[]> map, heightmap;
	unsigned w, h;
	int left, bottom, right, top;

	Map() : map() {}
	Map(unsigned w, unsigned h);

	/** Update viewport limits. */
	void reshape(int x, int y, int w, int h);
	void resize(MapSize size);
private:
	void resize(unsigned w, unsigned h);
	void resize(unsigned size) { resize(size, size); }
};

/**
 * Elementary world unit. This includes gaia stuff (e.g. trees, gold, berry
 * bushes...)
 */
class Unit {
protected:
	unsigned hp;

public:
	Point pos;
	// tile displacement
	int dx, dy;
	unsigned size;
	static unsigned count; // FIXME debug stuff, remove when done

protected:
	const AnimationTexture &animation;
	unsigned image_index;
	unsigned color;

public:
	Unit(
		unsigned hp, int x, int y,
		unsigned size,
		unsigned sprite_index,
		unsigned color,
		int dx=0, int dy=0
	);
	virtual ~Unit() { --count; }

	virtual void draw(Map &map) const;
	void draw_selection(Map &map) const;

	friend bool operator==(const Unit &lhs, const Unit &rhs) {
		return lhs.pos == rhs.pos && lhs.dx == rhs.dx && lhs.dy == rhs.dy;
	}
};

class Building final : public Unit {
	const AnimationTexture &overlay;
	unsigned overlay_index;
public:
	Building(
		unsigned id, unsigned p_id,
		int x, int y, unsigned size, unsigned color
	);
	void draw(Map &map) const override;
};

class Quadtree final {
	std::unique_ptr<Quadtree[]> children;
	AABB bounds;
	std::vector<std::shared_ptr<Unit>> objects;
	bool split;
public:
	Quadtree(AABB bounds = {})
		: children(), bounds(bounds), objects(), split(false) {}

	bool put(std::shared_ptr<Unit> obj);
	bool erase(Unit *obj);
	void clear();

	void query(std::vector<std::weak_ptr<Unit>> &lst, AABB bounds);
private:
	void reshape(AABB bounds) { this->bounds = bounds; }
};
