/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "base/types.hpp"
#include "drs.hpp"

#include <SDL2/SDL_mouse.h>

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

class Cursor final {
	std::unique_ptr<SDL_Cursor, decltype(&SDL_FreeCursor)> handle;
	Animation &icons;
public:
	Cursor(CursorId);
	~Cursor();

	void change(CursorId);
};

}
