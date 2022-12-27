#include "gfx.hpp"

#include <cassert>
#include <tracy/Tracy.hpp>

namespace aoe {
namespace gfx {

Tileset::Tileset() : imgs(), surf(nullptr, SDL_FreeSurface) {}

void Tileset::write(GLuint tex) {
	ZoneScoped;
	assert(surf.get());

	glBindTexture(GL_TEXTURE_2D, tex); // set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLenum mode = GL_RGBA;

	std::vector<uint32_t> data;
	data.reserve(surf->w * surf->h);
	uint32_t *pixels = (uint32_t*)surf->pixels;

	for (int y = 0, h = surf->h, p = surf->pitch >> 2; y < h; ++y)
		for (int x = 0, w = surf->w; x < w; ++x)
			data.emplace_back(pixels[y * p + x]);

	glTexImage2D(GL_TEXTURE_2D, 0, mode, surf->w, surf->h, 0, mode, GL_UNSIGNED_BYTE, data.data());

	surf.reset();
}

}
}
