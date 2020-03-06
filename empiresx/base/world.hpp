/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "random.hpp"
#include "math.hpp"

#include <cassert>

#include <array>
#include <memory>
#include <vector>
#include <set>

namespace genie {

struct StartMatch;

namespace game {

enum class TileId {
	// FIXME support forest, water and borders
	FLAT1,
	FLAT2,
	FLAT3,
	FLAT4,
	FLAT5,
	FLAT6,
	FLAT7,
	FLAT8,
	FLAT9,
	HILL_CORNER_SOUTH_EAST1,
	HILL_CORNER_NORTH_WEST1,
	HILL_CORNER_SOUTH_WEST1,
	HILL_CORNER_NORTH_EAST1,
	HILL_SOUTH,
	HILL_WEST,
	HILL_EAST,
	HILL_NORTH,
	HILL_CORNER_SOUTH_EAST2,
	HILL_CORNER_NORTH_WEST2,
	HILL_CORNER_SOUTH_WEST2,
	HILL_CORNER_NORTH_EAST2,
	FLAT_CORNER_SOUTH_EAST,
	FLAT_CORNER_NORTH_WEST,
	FLAT_CORNER_SOUTH_WEST,
	FLAT_CORNER_NORTH_EAST,
	TILE_MAX
};

/**
* Simple minimal API for a point in two dimensional euclidean space
*/

template<typename T> class Vector2 {
public:
	T x, y;

	constexpr Vector2<T>(T x=0, T y=0) noexcept : x(x), y(y) {}

	friend constexpr Vector2<T> operator+(Vector2<T> lhs, const Vector2<T> &rhs) noexcept {
		lhs += rhs;
		return lhs;
	}

	constexpr Vector2<T> &operator+=(const Vector2<T> &other) noexcept {
		x += other.x;
		y += other.y;
		return *this;
	}

	friend constexpr Vector2<T> operator-(Vector2<T> lhs, const Vector2<T>& rhs) noexcept {
		lhs -= rhs;
		return lhs;
	}

	constexpr Vector2<T> &operator-=(const Vector2<T>& other) noexcept {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	friend constexpr Vector2<T> operator/(Vector2<T> vec, T v) noexcept {
		vec /= v;
		return vec;
	}

	constexpr Vector2<T> &operator/=(T v) noexcept {
		x /= v;
		y /= v;
		return *this;
	}
};

enum class Quadrant {
	tr = 0,
	tl = 1,
	bl = 2,
	br = 3,
	bad = 4,
};

template<typename T> class Box2 {
public:
	T left, top, w, h; /**< width and height must be positive. */

	constexpr Box2(T left=0, T top=0, T w=0, T h=0) noexcept
		: left(left), top(top), w(w), h(h) { assert(w >= 0 && h >= 0); }

	constexpr Box2(const Vector2<T> &pos, const Vector2<T> &size) noexcept
		: left(pos.x), top(pos.y), w(size.x), h(size.y) { assert(size.x >= 0 && size.y >= 0); }

	constexpr T right() const noexcept { return left + w; }
	constexpr T bottom() const noexcept { return top + h; }
	constexpr Vector2<T> topleft() const noexcept { return Vector2<T>(left, top); }
	constexpr Vector2<T> rightbottom() const noexcept { return Vector2<T>(right(), bottom()); }
	constexpr Vector2<T> center() const noexcept { return Vector2<T>(left + w / 2, top + h / 2); }

	constexpr bool contains(const Box2<T> &box) const noexcept {
		return left <= box.left && top <= box.top && right() <= box.right() && bottom() <= box.bottom();
	}

	constexpr bool contains(const Vector2<T> &pt) const noexcept {
		return pt.x >= left && pt.y >= top && pt.x < right() && pt.y < bottom();
	}

	constexpr bool intersects(const Box2<T> &box) const noexcept {
		return !(left >= box.right() || top >= box.bottom() || right() <= box.left || bottom() <= box.top);
	}

	constexpr Quadrant quadrant(const Vector2<T> &pt) const noexcept {
		if (!contains(pt))
			return Quadrant::bad;

		if (pt.y < center().y) {
		}
	}
};

class Map final {
public:
	unsigned w, h;
	std::unique_ptr<uint8_t[]> tiles, heights; // y,x order

	Map(LCG &lcg, const StartMatch &settings);
};

enum class ResourceType {
	wood,
	food,
	gold,
	stone,
};

enum class GatherStatus {
	ok,
	incompatible,
	depleted,
};

class Resource {
protected:
	ResourceType type;
	unsigned amount;
public:
	Resource(ResourceType type, unsigned amount) : type(type), amount(amount) {}

	GatherStatus gather(Resource &dest, unsigned amount=1);
};

extern unsigned particle_id_counter;

template<typename T> class Particle {
protected:
	Box2<T> pos;
	unsigned anim_index;
	unsigned image_index;

	unsigned color, id;


	Particle(const Box2<T> &pos, unsigned anim_index, unsigned image_index=0, unsigned color=0)
		: pos(pos), anim_index(anim_index), image_index(image_index), color(color), id(particle_id_counter++) {}
public:
	virtual void draw(Map &map) const = 0;

	friend bool operator==(const Particle<T> &lhs, const Particle<T> &rhs) {
		return lhs.id == rhs.id;
	}

	friend bool operator==(const Particle<T> &lhs, unsigned id) {
		return lhs.id == id;
	}
};

/**
 * Elementary world unit. This includes gaia stuff (e.g. trees, gold, berry
 * bushes...)
 */
template<typename T> class Unit : public Particle<T> {
public:
	unsigned hp, hp_max;

	Unit(unsigned hp, const Box2<T> &pos, unsigned anim_index, unsigned image_index=0, unsigned color=0)
		: hp(hp), hp_max(hp), Particle<T>(pos, anim_index, image_index, color) {}
};

class StaticResource final : public Unit<int>, public Resource {
public:
	StaticResource(const Box2<int> &pos, ResourceType type, unsigned res_anim=0);
};

template<typename T> class Quadtree {
	struct Quadnode {
		std::array<std::unique_ptr<Quadnode>, 4> children;
		// XXX it is tempting to use a set, but this will probably degrade performance and mess too much with data caches
		std::vector<std::unique_ptr<Particle<T>>> items;
	};

	std::unique_ptr<Quadnode> root;
	Box2<T> bounds;
public:
	Quadtree(const Box2<T> &box) : bounds(box), root(nullptr) {}

	Quadtree(const Vector2<T> &size) : Quadtree(Box2<T>(0, 0, size.x, size.y)) {}

	Quadnode *closest_node(Box2<T> &bnds, const Vector2<T> &pt) {
		return nullptr;
	}

	void add(Particle<T> *item) {
	}
};

/** Container for all particles, entities, etc. */
class World final {
public:
	Map map;

	Quadtree<int> tiled_objects;
	Quadtree<float> movable_objects; // a.k.a. sprites

	World(LCG &lcg, const StartMatch &settings);
};

}

}
