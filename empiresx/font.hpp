#pragma once

/**
 * True Type Font rendering subsystem
 *
 * This is more complicated than one would expect...
 * Since libfreetype does not properly render any TTF type I throw at it,
 * we will use windows native GDI rendering instead to improve font quality.
 */

#include "os_macros.hpp"

#include <memory>

#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_ttf.h>

#include "render.hpp"

struct SDL_Texture;

namespace genie {

class Font final {
    std::unique_ptr<TTF_Font, decltype(&TTF_CloseFont)> handle;
public:
    const int ptsize;

    Font(const char* fname, int ptsize);
    Font(TTF_Font* handle, int ptsize = 12);

    SDL_Surface* surf_solid(const char* text, SDL_Color fg);
    SDL_Texture* tex_solid(SimpleRender& r, const char* text, SDL_Color fg);

    TTF_Font* data() { return handle.get(); }
};

}

class TTF final {
public:
	TTF();
	~TTF();
};