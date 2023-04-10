#include "gfx.hpp"

#include <cassert>
#include <cstring>
#include <tracy/Tracy.hpp>

namespace aoe {
namespace gfx {

Tileset::Tileset() : imgs(), surf(nullptr, SDL_FreeSurface), w(0), h(0) {}

void Tileset::write(GLuint tex) {
	ZoneScoped;
	assert(surf.get());

	w = surf->w;
	h = surf->h;

	{
		ZoneScopedN("prepare");
		glBindTexture(GL_TEXTURE_2D, tex); // set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	GLenum mode = GL_RGBA;

	//surf->w = std::min(4096 * 2, surf->w);

	std::vector<uint32_t> data;
	{
		ZoneScopedN("resize");
		data.resize(surf->w * surf->h);
	}
	{
		ZoneScopedN("copy");
		uint32_t *pixels = (uint32_t*)surf->pixels;
		size_t pos = 0;

		for (int y = 0, h = surf->h, p = surf->pitch >> 2; y < h; ++y) {
			memcpy(&data[pos], &pixels[y * p], 4 * surf->w);
			pos += surf->w;
		}
	}
	{
		ZoneScopedN("flush");
		glTexImage2D(GL_TEXTURE_2D, 0, mode, surf->w, surf->h, 0, mode, GL_UNSIGNED_BYTE, data.data());
	}

	surf.reset();
}

}
}
