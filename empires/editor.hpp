/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Scenario editor
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#pragma once

class Editor {
	bool run;
	int x, y, w, h;
public:
	Editor();
	~Editor();

	void reshape(int x, int y, int w, int h) {
		this->x = x; this->y = y;
		this->w = w; this->h = h;
	}

	void idle();

	void start();
	void stop();
	void draw();
};

extern Editor editor;
