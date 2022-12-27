#include "gfx.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

namespace aoe {
namespace gfx {

TextureBuilder::TextureBuilder() : textures() {}

IdPoolRef TextureBuilder::add_img(unsigned w, unsigned h) {
	if (w > UINT16_MAX || h > UINT16_MAX)
		throw std::runtime_error("image too big");

	auto ins = textures.emplace(SDL_Rect{ 0, 0, (int)w, (int)h });
	if (!ins.second)
		throw std::runtime_error("cannot add image");

	return ins.first->first;
}

std::vector<TextureRef> TextureBuilder::collect(unsigned &w, unsigned &h) {
	unsigned max_size = std::min(STBRP__MAXVAL, INT_MAX);

	if (w > max_size || h > max_size)
		throw std::runtime_error("target texture too big");

	stbrp_context ctx{ 0 };

	std::vector<IdPoolRef> id_to_ref; // keep track of refs
	std::vector<stbrp_rect> rects;
	
	rects.reserve(w);

	for (auto kv : textures) {
		TextureRef &ref = kv.second;
		stbrp_rect rect{ id_to_ref.size(), ref.bnds.w, ref.bnds.h, 0, 0, 0 };
		rects.push_back(rect);
		id_to_ref.emplace_back(kv.first);
	}

	std::vector<stbrp_node> nodes(w);
	stbrp_init_target(&ctx, w, h, nodes.data(), nodes.size());

	stbrp_setup_heuristic(&ctx, STBRP_HEURISTIC_Skyline_BF_sortHeight);
	stbrp_setup_allow_out_of_mem(&ctx, 0);

	if (!stbrp_pack_rects(&ctx, rects.data(), rects.size()))
		throw std::runtime_error("cannot fit images in texture");

	std::vector<TextureRef> texs;

	w = h = 0;

	// traverse result to restore refs
	for (size_t i = 0; i < rects.size(); ++i) {
		stbrp_rect &r = rects[i];
		texs.emplace_back(id_to_ref[i], SDL_Rect{ r.x, r.y, r.w, r.h });
		w = std::max<unsigned>(w, r.w);
		h = std::max<unsigned>(h, r.h);
	}

	return texs;
}

}
}
