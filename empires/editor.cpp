/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Scenario editor
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "editor.hpp"
#include "game.hpp"

#include "scenario.hpp"
#include <genie/dbg.h>

Editor editor;
GameConfig editor_gamecfg;

Editor::Editor() : run(false), x(0), y(0), w(640), h(480) {}
Editor::~Editor() {}

void Editor::idle(unsigned ms) {
	game.idle(ms);
}

void Editor::start(const char *path) {
	run = true;
	if (path) {
		Scenario scn(path);
		scn.dump();
	}
	game.reset(editor_gamecfg);
	game.reshape(x, y, w, h);
}

void Editor::stop() {
	run = false;
	game.stop();
}

void Editor::draw() {
	game.draw();
}
