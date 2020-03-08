/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "game.hpp"
#include "engine.hpp"

#include <cassert>

namespace genie {

ImageCache *cache;

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

namespace game {

void img_dim(game::Box2<float> &dim, unsigned res, unsigned image) {
	Animation &anim = const_cast<Animation&>(cache->get(res));
	auto &img = anim.subimage(image);

	dim.left += tw / 2 - img.hotspot_x;
	dim.top += th / 2 - img.hotspot_y;
	dim.w = static_cast<float>(img.texture.width);
	dim.h = static_cast<float>(img.texture.height);
}

void Particle::draw(int offx, int offy) const {
	SimpleRender &r = (SimpleRender&)eng->w->render();
	Animation &anim = const_cast<Animation&>(cache->get(this->anim_index));
	anim.subimage(this->image_index).draw(r, static_cast<int>(this->scr.left) + offx, static_cast<int>(this->scr.top) + offy);
}

}

}
