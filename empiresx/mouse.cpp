/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "mouse.hpp"

#include <cassert>

#include "engine.hpp"
#include "drs.hpp"

#include <SDL2/SDL_syswm.h>

namespace genie {

Cursor::Cursor(CursorId id) : handle(nullptr, &SDL_FreeCursor) {
	change(id);
}

Cursor::~Cursor() {
	change(CursorId::os_default);
}

static bool is_clipping = false;

void Cursor::change(CursorId id) {
	if (!(unsigned)id) {
		SDL_SetCursor(SDL_GetDefaultCursor());
		handle.reset();
		return;
	}

	SDL_Cursor *newhandle;
	Animation icons(eng->assets->open_slp(eng->assets->pal_default, 51000));

	auto &img = icons.subimage((unsigned)id - 1);
	if (!(newhandle = SDL_CreateColorCursor(img.surface.data(), img.hotspot_x, img.hotspot_y))) {
		fprintf(stderr, "could not change cursor to %u\n", (unsigned)id);
		return;
	}

	SDL_SetCursor(newhandle);
	handle.reset(newhandle);
}

void Cursor::clip() { clip(is_clipping); }

void Cursor::clip(bool enable) {
#if windows
	if (!enable) {
		ClipCursor(NULL);
		is_clipping = false;
		return;
	}

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWindowWMInfo(eng->w->data(), &info)) {
		fprintf(stderr, "%s: cannot get window info: %s\n", __func__, SDL_GetError());
		return;
	}

	HWND h = info.info.win.window;
	RECT r;
	POINT p{ 0, 0 };
	if (!ClientToScreen(h, &p) || !GetClientRect(h, &r)) {
		fprintf(stderr, "%s: cannot get clipping bounds\n", __func__);
		return;
	}
	// adjust clipping area to window screen area
	r.left += p.x; r.right += p.x;
	r.top += p.y; r.bottom += p.y;
	ClipCursor(&r);
	is_clipping = true;
#else
	fprintf(stderr, "%s: stub\n", __func);
#endif
}

}
