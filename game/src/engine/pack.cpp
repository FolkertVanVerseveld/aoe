#include "gfx.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#include <tracy/Tracy.hpp>

namespace aoe {
namespace gfx {

ImageRef::ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, int hotspot_x, int hotspot_y) : ImageRef(ref, bnds, surf, hotspot_x, hotspot_y, 0.0f, 1.0f, 0.0f, 1.0f) {}

ImageRef::ImageRef(IdPoolRef ref, const SDL_Rect &bnds, SDL_Surface *surf, int hotspot_x, int hotspot_y, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1)
	: ref(ref), bnds(bnds)
	, hotspot_x(hotspot_x), hotspot_y(hotspot_y), s0(s0), t0(t0), s1(s1), t1(t1), surf(surf) {}

ImagePacker::ImagePacker() : images() {}

IdPoolRef ImagePacker::add_img(int hotspot_x, int hotspot_y, SDL_Surface *surf) {
	ZoneScoped;

	assert(surf->w >= 0 && surf->h >= 0);
	unsigned w = surf->w, h = surf->h;

	if (w > UINT16_MAX || h > UINT16_MAX)
		throw std::runtime_error("image too big");

	auto ins = images.emplace(SDL_Rect{ 0, 0, 0, 0 }, surf, hotspot_x, hotspot_y);
	if (!ins.second)
		throw std::runtime_error("cannot add image");

	return ins.first->first;
}

Tileset ImagePacker::collect(int w, int h) {
	ZoneScoped;

	if (w < 1 || h < 1)
		throw std::runtime_error("dimensions must be positive");

	// limit pack size to stbrp limits
	int max_size = std::min(STBRP__MAXVAL, INT_MAX);

	if (w > max_size || h > max_size)
		throw std::runtime_error("target texture too big");

	stbrp_context ctx{ 0 };

	std::vector<IdPoolRef> id_to_ref; // keep track of refs
	std::vector<stbrp_rect> rects;
	
	rects.reserve(w);

	// add all rects to be packed
	for (auto kv : images) {
		ImageRef &ref = kv.second;
		stbrp_rect rect{ id_to_ref.size(), ref.surf->w, ref.surf->h, 0, 0, 0 };
		rects.push_back(rect);
		id_to_ref.emplace_back(kv.first);
	}

	{
		ZoneScopedN("pack");
		std::vector<stbrp_node> nodes(w);
		stbrp_init_target(&ctx, w, h, nodes.data(), nodes.size());

		stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
		stbrp_setup_allow_out_of_mem(&ctx, 0);

		// do pack magic
		if (!stbrp_pack_rects(&ctx, rects.data(), rects.size()))
			throw std::runtime_error("cannot fit images in texture");
	}

	Tileset ts;

	{
		ZoneScopedN("resize");
		w = h = 0;

		// adjust w and h to ensure all images are inclosed in a minimal rectangle
		for (stbrp_rect &r : rects) {
			w = std::max<unsigned>(w, r.x + r.w);
			h = std::max<unsigned>(h, r.y + r.h);
		}
	}

	{
		ZoneScopedN("restore refs");
		// traverse result to restore refs
		for (size_t i = 0; i < rects.size(); ++i) {
			stbrp_rect &r = rects[i];

			// compute texture coordinates
			GLfloat s0 = (GLfloat)r.x / w, t0 = (GLfloat)r.y / h;
			GLfloat s1 = (GLfloat)(r.x + r.w) / w, t1 = (GLfloat)(r.y + r.h) / h;

			const ImageRef &ref = images.at(id_to_ref[i]);

			ts.imgs.emplace(id_to_ref[i], SDL_Rect{ r.x, r.y, r.w, r.h }, nullptr, ref.hotspot_x, ref.hotspot_y, s0, t0, s1, t1);
		}
	}

	{
		ZoneScopedN("copy");
		ts.surf.reset(SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32));

		std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> tmp(nullptr, SDL_FreeSurface);

		// traverse again to copy data to big surface
		for (const ImageRef &r : ts.imgs) {
			SDL_Surface *surf = images.at(r.ref).surf;
			tmp.reset(SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0));
			if (!tmp)
				throw std::runtime_error("cannot convert image");

			uint32_t *pixels1 = (uint32_t*)ts.surf->pixels;
			const uint32_t *pixels0 = (const uint32_t*)tmp->pixels;

			int p1 = ts.surf->pitch >> 2, p0 = tmp->pitch >> 2;

			// copy data
			for (int y0 = 0, y1 = r.bnds.y, h = tmp->h; y0 < h; ++y0, ++y1)
				// TODO use memcpy?
				for (int x0 = 0, x1 = r.bnds.x, w = tmp->w; x0 < w; ++x0, ++x1)
					pixels1[y1 * p1 + x1] = pixels0[y0 * p0 + x0];
		}
	}

	return ts;
}

}
}
