/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "types.hpp"
#include "random.hpp"
#include "math.hpp"
#include "geom.hpp"

#include <cassert>

#include <array>
#include <memory>
#include <vector>
#include <set>
#include <algorithm>
#include <deque>

namespace genie {

struct StartMatch;
class Multiplayer;

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

		if (pt.y < center().y)
			return pt.x < center().x ? Quadrant::tl : Quadrant::tr;

		return pt.x < center().x ? Quadrant::bl : Quadrant::br;
	}
};

extern void img_dim(Box2<float> &dim, int &hotspot_x, int &hotspot_y, unsigned res, unsigned image);

class Map final {
public:
	unsigned w, h;
	std::unique_ptr<uint8_t[]> tiles, heights; // y,x order

	Map(LCG &lcg, const StartMatch &settings);

	Box2<float> tile_to_scr(const Vector2<float> &pos, int &hotspot_x, int &hotspot_y, unsigned res, unsigned image) {
		Box2<float> scr;

		genie::tile_to_scr(scr.left, scr.top, pos.x, pos.y);
		img_dim(scr, hotspot_x, hotspot_y, res, image);

		return scr;
	}
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

extern uint32_t particle_id_counter;

/**
 * Most abstract non-tiled world object. This may be an effect (e.g. debris, dead units, ...)
 * or static resources (trees, berry bushes, ...) or dynamic stuff (e.g. deer, clubman, ...)
 */
class Particle {
public:
	Box2<float> pos, scr;
	int hotspot_x, hotspot_y;
protected:
	unsigned anim_index;
	unsigned image_index;
	unsigned color;
	uint32_t id;
	bool hflip;

	// special ctor for e.g. effects that do not care about the tile position \a pos
	Particle(const Box2<float> &scr, unsigned anim_index, unsigned image_index=0, unsigned color=0, bool hflip=false)
		: pos(), scr(scr), anim_index(anim_index), image_index(image_index), color(color)
		, id(particle_id_counter), hflip(hflip)
	{
		// disallow particle_id_counter to be zero
		particle_id_counter = particle_id_counter == UINT32_MAX ? 1 : particle_id_counter + 1;
	}

	// default ctor for anything that is not a graphical effect
	Particle(Map &map, const Box2<float> &pos, unsigned anim_index, unsigned image_index=0, unsigned color=0, bool hflip=false)
		: pos(pos), scr(map.tile_to_scr(pos.topleft(), hotspot_x, hotspot_y, anim_index, image_index)), anim_index(anim_index), image_index(image_index), color(color)
		, id(particle_id_counter), hflip(hflip)
	{
		// disallow particle_id_counter to be zero
		particle_id_counter = particle_id_counter == UINT32_MAX ? 1 : particle_id_counter + 1;
	}
public:
	virtual ~Particle() {}

	constexpr uint32_t getid() const noexcept {
		return id;
	}

protected:
	void draw(int offx, int offy, unsigned index) const;
public:
	virtual void draw(int offx, int offy) const {
		draw(offx, offy, image_index);
	}

	friend bool operator==(const Particle &lhs, const Particle &rhs) {
		return lhs.id == rhs.id;
	}

	friend bool operator==(const Particle &lhs, unsigned id) {
		return lhs.id == id;
	}
};

class World;

/** Minimal wrapper to indicate that the Particle has some logic that is executed every game step. */
class Active {
public:
	virtual ~Active() {}

	virtual void tick(World &world) = 0;
};

enum class EffectType {
	move,
	marker, // waypoint
	explosion0, // small
	explosion1, // medium
	explosion2, // big
};

/** Special foreground graphical effect */
class Effect final : public Particle, public Active {
public:
	Effect(Map &map, const Box2<float> &pos, EffectType type);
};

#undef min

class Alive : public Active {
public:
	unsigned hp, hp_max;

	Alive(unsigned hp=1) : hp(hp), hp_max(hp) { assert(hp); }
	virtual ~Alive() {}

	constexpr Alive &operator+=(unsigned v) noexcept {
		hp = std::min(hp + v, hp_max);
		return *this;
	}

	constexpr Alive &operator-=(unsigned v) noexcept {
		hp = v > hp ? 0 : hp - v;
		return *this;
	}
};

enum class UnitType {
	villager,
	clubman,
};

class Production final {
public:
	UnitType what;
	unsigned ticks, total;

	Production(UnitType what, unsigned ticks);

	bool tick();
};

enum class BuildingType {
	barracks,
	town_center
};

class Building final : public Particle, public Alive {
	unsigned anim_player;
	unsigned player;
	std::deque<Production> prod;

public:
	const BuildingType type;

	Building(Map &map, const Box2<float> &pos, BuildingType type, unsigned player=0);

	void tick(World &world) override;
	void draw(int offx, int offy) const override;
};

class StaticResource final : public Particle, public Resource {
public:
	StaticResource(Map &map, const Box2<float> &pos, ResourceType type, unsigned res_anim, unsigned image=0);
};

enum class UnitDirection {
	down,
	down_left,
	left,
	top_left,
	top,
	// mirrored images
	top_right,
	right,
	down_right
};

class Unit : public Particle, public Alive {
	UnitType type;
	UnitDirection dir; /**< indicates which direction the unit is facing */
	unsigned dir_images;
	float movespeed;
	Vector2<float> target; /**< map pos target. this never represents a screen position! */
public:
	Unit(Map &map, const Box2<float> &pos, UnitType type, unsigned player);
	virtual ~Unit() {}

	virtual void imgtick();
	virtual void tick(World &world) override;
	virtual void draw(int offx, int offy) const override;
};

class Villager final : public Unit {
public:
	Villager(Map &map, const Box2<float> &pos, unsigned player);
};

/** Container for all particles, entities, etc. */
class World final {
public:
	Map map;
	LCG &lcg;
	bool host;

private:
	std::vector<std::unique_ptr<StaticResource>> static_res;
	std::vector<std::unique_ptr<Building>> buildings;
	std::vector<std::unique_ptr<Unit>> units;

public:
	World(LCG &lcg, const StartMatch &settings, bool host);

	void populate(unsigned players);
	/**
	 * Animate all dynamic particles. This is not synchronized with the server whatsoever,
	 * since there is no need to (well, it should be in sync automatigcally... but we have
	 * to see if that also happens in practise).
	 */
	void imgtick();
	/**
	 * Compute the next simulation step and use the mp for any events that are generated.
	 * If the world is created as host, this will also send messages to other clients.
	 */
	void tick();

	void query_static(std::vector<Particle*> &list, const Box2<float> &bounds);
	// FIXME change type to Unit*
	void query_dynamic(std::vector<Particle*> &list, const Box2<float> &bounds);
};

}

}
