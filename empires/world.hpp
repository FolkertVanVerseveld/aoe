/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Virtual world logic model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#pragma once

#include "image.hpp"

#include <vector>

/**
 * Elementary world unit. This includes gaia stuff (e.g. trees, gold, berry
 * bushes...)
 */
class Unit {
protected:
	unsigned hp;

	int x, y;
	unsigned w, h;

	const AnimationTexture &animation;
	unsigned image_index;

public:
	Unit(
		unsigned hp, int x, int y,
		unsigned w, unsigned h,
		unsigned sprite_index
	);
	virtual ~Unit() {}

	virtual void draw(unsigned color) const;
};

class Building final : public Unit {
	const AnimationTexture &overlay;
	unsigned overlay_index;
public:
	Building(unsigned id, unsigned p_id, int x, int y);
	void draw(unsigned color) const override;
};

// TODO implement quadtree

struct Point final {
	int x, y;

	Point() : x(0), y(0) {}
	Point(int x, int y) : x(x), y(y) {}

	friend bool operator==(const Point &lhs, const Point &rhs) {
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}
};

struct AABB final {
	Point pos, hbounds;

	AABB(Point pos = {}, Point hbounds = Point(1, 1))
		: pos(pos), hbounds(hbounds) {}

	AABB(int size) : AABB({0, 0}, {size, size}) {}

	bool contains(const Point &p) const {
		return p.x < pos.x + hbounds.x && p.x >= pos.x - hbounds.x
			&& p.y < pos.y + hbounds.y && p.y >= pos.y - hbounds.y;
	}

	bool intersects(const AABB &other) const {
		return (pos.x + hbounds.x >= other.pos.x - other.hbounds.x || pos.x - hbounds.x < other.pos.x + other.hbounds.x)
			&& (pos.y + hbounds.y >= other.pos.y - other.hbounds.y || pos.y - hbounds.y < other.pos.y + other.hbounds.y);
	}
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

	// XXX use weakptr?
	void query(std::vector<std::shared_ptr<Unit>> &lst);
private:
	void reshape(AABB bounds) { this->bounds = bounds; }
};
