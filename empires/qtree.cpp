/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Virtual world logic model
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "world.hpp"

bool Quadtree::put(std::shared_ptr<Unit> obj) {
	// FIXME split and unsplit quadtree
	objects.push_back(obj);
	return true;
}

bool Quadtree::erase(Unit *obj) {
	// FIXME check bounds
	// TODO traverse children amongst other things

	for (auto it = objects.begin(); it != objects.end(); ++it) {
		auto o = *it;

		if (*obj == *o) {
			objects.erase(it);
			return true;
		}
	}

	return false;
}
