/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "base/types.hpp"

#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_rect.h>

#include <memory>

namespace genie {

enum class CursorId {
	os_default,
	game_default,
	wait,
	gather, // unused
	task,
	attack,
	convert_heal,
	help,
};

extern class ClipControl final {
	/** Clipping area that is considered valid iff area.h != 0. */
	SDL_Rect area;
	bool clipping, enhanced;
public:
	ClipControl() : area(), clipping(false), enhanced(false) {}

	/** Disable cursor clipping. */
	void noclip();
	/** Enable cursor clipping and determine clipping area. */
	void clip(bool enhanced);
	/** Enable cursor clipping around specified area. */
	void clip(const SDL_Rect &bnds);

	/** Reapply focus state. This should be called whenever the window has lost and regained focus. */
	void focus_gained();
} clip_control;

class Cursor final {
	std::unique_ptr<SDL_Cursor, decltype(&SDL_FreeCursor)> handle;
public:
	Cursor(CursorId);
	~Cursor();

	void change(CursorId);
};

}
