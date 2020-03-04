/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "render.hpp"
#include "drs.hpp"

#include "base/game.hpp"

#include <map>

namespace genie {

class ImageCache final {
	Palette pal;
	std::map<res_id, Animation> cache;
public:
	static constexpr res_id default_palette = 50500;
	/** Construct animation cache for in-game graphics that is frequently used. */
	ImageCache();
	/** Nuke saved animations. */
	void clear();
	/**
	 * Fetch slp from cache (if the cache does not have it, it is created and returned).
	 * The specified \a id must be valid.
	 */
	Animation &get(res_id id);
};

}
