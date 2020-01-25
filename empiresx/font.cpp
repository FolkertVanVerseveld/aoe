#include "font.hpp"

#include <SDL2/SDL_render.h>

#include <cassert>
#include <stdexcept>
#include <string>

#include "engine.hpp"

namespace genie {

Font::Font(const char* fname, int ptsize) : handle(NULL, &TTF_CloseFont), ptsize(ptsize) {
	TTF_Font* f;

	if (!(f = TTF_OpenFont(fname, ptsize)))
		throw std::runtime_error(std::string("Unable to open font \"") + fname + "\": " + TTF_GetError());

	handle.reset(f);
}

Font::Font(TTF_Font* handle, int ptsize) : handle(handle, &TTF_CloseFont), ptsize(ptsize) {}

SDL_Surface* Font::surf(const char* text, SDL_Color fg) {
	return text && *text ? TTF_RenderText_Blended(handle.get(), text, fg) : nullptr;
}

SDL_Texture* Font::tex(SimpleRender& r, const char* text, SDL_Color fg) {
	if (!text || !*text)
		return nullptr;
	std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> s(surf(text, fg), &SDL_FreeSurface);
	return SDL_CreateTextureFromSurface(r.canvas(), s.get());
}

Text::Text(SimpleRender& r, Font& f, const std::string& s, SDL_Color fg) : m_tex(0, 0, NULL), str(s) {
	Surface surf(f.surf(s.c_str(), fg));
	m_tex.reset(r, surf.data());
}

Text::Text(SimpleRender &r, const std::string &s, SDL_Color fg) : Text(r, eng->assets->fnt_default, s, fg) {}

void Text::paint(SimpleRender& r, int x, int y) {
	m_tex.paint(r, x, y);
}

TextBuf::TextBuf(SimpleRender& r, Font& f, const std::string& s, SDL_Color fg) : r(r), f(f), fg(fg), txt(nullptr) {
	if (s.size())
		txt.reset(new Text(r, f, s, fg));
}

TextBuf::TextBuf(SimpleRender& r, const std::string& s, SDL_Color fg) : TextBuf(r, eng->assets->fnt_default, s, fg) {}

void TextBuf::append(int ch) {
	if (!txt.get())
		str(std::string(1, ch), fg);
	else
		str(txt->str + std::string(1, ch), fg);
}

void TextBuf::erase() {
	if (!txt.get() || txt->str.size() == 1)
		clear();
	else {
		std::string s = txt->str;
		s.pop_back();
		str(s, fg);
	}
}

void TextBuf::clear() {
	txt.reset(nullptr);
}

void TextBuf::str(const std::string &s, SDL_Color fg) {
	txt.reset(new Text(r, f, s, fg));
}

const std::string &TextBuf::str() const {
	if (txt.get()) {
		assert(txt->str.size());
		return txt->str;
	}
	return "";
}

void TextBuf::paint(int x, int y) {
	if (txt.get())
		txt->paint(r, x, y);
}

}