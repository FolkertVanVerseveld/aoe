/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "mouse.hpp"

#include <cassert>

#include "engine.hpp"

namespace genie {

Cursor::Cursor(CursorId id) : handle(nullptr, &SDL_FreeCursor), icons(eng->assets->open_slp(eng->assets->pal_default, 51000)) {
	change(id);
}

Cursor::~Cursor() {
	change(CursorId::os_default);
}

void Cursor::change(CursorId id) {
	if (!(unsigned)id) {
		SDL_SetCursor(SDL_GetDefaultCursor());
		handle.reset();
		return;
	}

	SDL_Cursor *newhandle;

	auto &img = icons.subimage((unsigned)id - 1);
	if (!(newhandle = SDL_CreateColorCursor(img.surface.data(), img.hotspot_x, img.hotspot_y))) {
		fprintf(stderr, "could not change cursor to %u\n", (unsigned)id);
		return;
	}

	SDL_SetCursor(newhandle);
	handle.reset(newhandle);
}

}
