#include "font.hpp"

#include <SDL2/SDL_render.h>

#include <stdexcept>
#include <string>

namespace genie {

Font::Font(const char* fname, int ptsize) : handle(NULL, &TTF_CloseFont), ptsize(ptsize) {
	TTF_Font* f;

	if (!(f = TTF_OpenFont(fname, ptsize)))
		throw std::runtime_error(std::string("Unable to open font \"") + fname + "\": " + TTF_GetError());

	handle.reset(f);
}

Font::Font(TTF_Font* handle, int ptsize) : handle(handle, &TTF_CloseFont), ptsize(ptsize) {}

SDL_Surface* Font::surf_solid(const char* text, SDL_Color fg) {
	return TTF_RenderText_Solid(handle.get(), text, fg);
}

SDL_Texture* Font::tex_solid(SimpleRender& r, const char* text, SDL_Color fg) {
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> s(surf_solid(text, fg), &SDL_FreeSurface);
	return SDL_CreateTextureFromSurface(r.canvas(), s.get());
}

}