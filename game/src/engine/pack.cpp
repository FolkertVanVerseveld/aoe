#include "gfx.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#include <tracy/Tracy.hpp>

namespace aoe {
namespace gfx {

ImageRef::ImageRef(IdPoolRef ref, int x, int y, SDL_Surface *surf) : ref(ref), x(x), y(y), surf(surf) {}

ImagePacker::ImagePacker() : images() {}

IdPoolRef ImagePacker::add_img(SDL_Surface *surf) {
	ZoneScoped;

	assert(surf->w >= 0 && surf->h >= 0);
	unsigned w = surf->w, h = surf->h;

	if (w > UINT16_MAX || h > UINT16_MAX)
		throw std::runtime_error("image too big");

	auto ins = images.emplace(0, 0, surf);
	if (!ins.second)
		throw std::runtime_error("cannot add image");

	return ins.first->first;
}

std::vector<ImageRef> ImagePacker::collect(unsigned &w, unsigned &h) {
	ZoneScoped;
	unsigned max_size = std::min(STBRP__MAXVAL, INT_MAX);

	if (w > max_size || h > max_size)
		throw std::runtime_error("target texture too big");

	stbrp_context ctx{ 0 };

	std::vector<IdPoolRef> id_to_ref; // keep track of refs
	std::vector<stbrp_rect> rects;
	
	rects.reserve(w);

	for (auto kv : images) {
		ImageRef &ref = kv.second;
		stbrp_rect rect{ id_to_ref.size(), ref.surf->w, ref.surf->h, 0, 0, 0 };
		rects.push_back(rect);
		id_to_ref.emplace_back(kv.first);
	}

	std::vector<stbrp_node> nodes(w);
	stbrp_init_target(&ctx, w, h, nodes.data(), nodes.size());

	stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
	stbrp_setup_allow_out_of_mem(&ctx, 0);

	if (!stbrp_pack_rects(&ctx, rects.data(), rects.size()))
		throw std::runtime_error("cannot fit images in texture");

	std::vector<ImageRef> imgs;

	w = h = 0;

	// traverse result to restore refs
	for (size_t i = 0; i < rects.size(); ++i) {
		stbrp_rect &r = rects[i];
		imgs.emplace_back(id_to_ref[i], r.x, r.y);

		// adjust w and h to ensure all images are inclosed in a minimal rectangle
		w = std::max<unsigned>(w, r.x + r.w);
		h = std::max<unsigned>(h, r.y + r.h);
	}

	return imgs;
}

}
}
