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
	bool is_clipping, clipping, enhanced;
public:
	ClipControl() : is_clipping(false), clipping(false), enhanced(false) {}

	/** Disable cursor clipping. */
	void noclip();
	/** Enable cursor clipping and determine clipping area. */
	void clip(bool enhanced);
	/** Enable cursor clipping around specified area. */
	void clip(const SDL_Rect &bnds);

	/**
	 * Reapply focus state. This should be called whenever the window has lost and regained focus.
	 * NOTE on linux, this will be spuriously called because clip() may have failed earlier!
	 */
	void focus_gained();
	void focus_lost();
} clip_control;

class Cursor final {
	std::unique_ptr<SDL_Cursor, decltype(&SDL_FreeCursor)> handle;
public:
	Cursor(CursorId);
	~Cursor();

	void change(CursorId);
};

}
