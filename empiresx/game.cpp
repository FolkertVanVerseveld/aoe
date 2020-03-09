/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#define DEBUG 1

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

void img_dim(game::Box2<float> &dim, int &hotspot_x, int &hotspot_y, unsigned res, unsigned image) {
	Animation &anim = const_cast<Animation&>(cache->get(res));
	auto &img = anim.subimage(image);

	hotspot_x = img.hotspot_x;
	hotspot_y = img.hotspot_y;

	dim.left += tw / 2 - img.hotspot_x;
	dim.top += th / 2 - img.hotspot_y;
	dim.w = static_cast<float>(img.texture.width);
	dim.h = static_cast<float>(img.texture.height);
}

void Particle::draw(int offx, int offy) const {
	SimpleRender &r = (SimpleRender&)eng->w->render();
	Animation &anim = const_cast<Animation&>(cache->get(this->anim_index));
	anim.subimage(this->image_index).draw(r, static_cast<int>(this->scr.left) + offx, static_cast<int>(this->scr.top) + offy);
#if DEBUG
	r.color(SDL_Color{0xff, 0, 0, SDL_ALPHA_OPAQUE});
	SDL_Rect bnds{static_cast<int>(this->scr.left) + offx, static_cast<int>(this->scr.top) + offy, static_cast<int>(scr.w), static_cast<int>(scr.h)};
	SDL_RenderDrawRect(r.canvas(), &bnds);
#endif
}

void Building::draw(int offx, int offy) const {
	Particle::draw(offx, offy);

	// FIXME improve code maintenance
	SimpleRender &r = (SimpleRender&)eng->w->render();
	Animation &base = const_cast<Animation&>(cache->get(this->anim_index));
	auto &base_img = base.subimage(this->image_index, player);
	base_img.draw(r, static_cast<int>(this->scr.left) + offx, static_cast<int>(this->scr.top) + offy);

	Animation &top = const_cast<Animation&>(cache->get(this->anim_player));
	auto &img = top.subimage(this->image_index, player);

	// reconstruct original position
	int dx, dy;
	dx = -(tw / 2 - base_img.hotspot_x);
	dy = -(th / 2 - base_img.hotspot_y);

	// adjust for player image
	dx += tw / 2 - img.hotspot_x;
	dy += th / 2 - img.hotspot_y;

	img.draw(r, static_cast<int>(this->scr.left) + offx + dx, static_cast<int>(this->scr.top) + offy + dy);
}

}

}
