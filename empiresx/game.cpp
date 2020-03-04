/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "game.hpp"
#include "engine.hpp"

#include <cassert>

namespace genie {

ImageCache::ImageCache() : pal(eng->assets->open_pal(ImageCache::default_palette)), cache() {}

void ImageCache::clear() {
	cache.clear();
}

Animation &ImageCache::get(res_id id) {
	// ask cache
	auto search = cache.find(id);
	if (search != cache.end())
		return const_cast<Animation&>(search->second);

	// load into cache
	cache.emplace(id, eng->assets->open_slp(pal, id));
	search = cache.find(id);
	assert(search != cache.end());
	return const_cast<Animation&>(search->second);
}

}
