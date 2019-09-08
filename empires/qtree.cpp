/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Virtual world logic model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include <genie/dbg.h>

#include "world.hpp"
#include "game.hpp"

void Point::to_screen(Point &dst) const {
	// TODO test if height computation is correct.
	uint8_t *height = game.map.heightmap.get();

	// convert tile to screen coordinates
	int tx = x, ty = y;

	dst.x = (tx + ty) * TILE_WIDTH + TILE_WIDTH;
	dst.y = (-ty + tx - height[ty * game.map.w + tx]) * TILE_HEIGHT + TILE_HEIGHT;
}

void Quadtree::clear() {
	assert(!split);
	dynamic_objects.clear();
	objects.clear();
}

bool Quadtree::put(Unit *obj) {
	// FIXME split and unsplit quadtree
	std::shared_ptr<Unit> unit = std::shared_ptr<Unit>(obj);

	DynamicUnit *dobj = dynamic_cast<DynamicUnit*>(obj);
	if (dobj)
		dynamic_objects.push_back(dobj);

	objects.push_back(unit);
	return true;
}

void Quadtree::update(Unit *obj) {
	// FIXME stub
	(void)obj;
}

bool Quadtree::erase(Unit *obj) {
	// FIXME check bounds
	// TODO traverse children amongst other things

	DynamicUnit *dobj = dynamic_cast<DynamicUnit*>(obj);
	if (dobj) {
		for (auto it = dynamic_objects.begin(); it != dynamic_objects.end(); ++it) {
			auto o = *it;

			if (*obj == *o) {
				dynamic_objects.erase(it);
				break;
			}
		}

		return false;
	}

	for (auto it = objects.begin(); it != objects.end(); ++it) {
		auto o = *it;

		if (*obj == *o) {
			objects.erase(it);
			return true;
		}
	}

	return false;
}

void Quadtree::query(std::vector<std::shared_ptr<Unit>> &lst, AABB bounds) {
	if (!this->bounds.intersects(bounds))
		return;

	for (auto &o : objects)
		if (bounds.contains(o->pos))
			// XXX verify this doesn't yield a temporary Unit
			// (i.e. it isn't destroyed when this goes out of scope)
			lst.emplace_back(o);
}
