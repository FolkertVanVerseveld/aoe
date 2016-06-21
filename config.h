#ifndef AOE_CONFIG_H
#define AOE_CONFIG_H

#define PATHFIND_LOW 0
#define PATHFIND_MEDIUM 1
#define PATHFIND_HIGH 2

struct config {
	unsigned screen_size;
	unsigned rollover_text;
	unsigned mouse_style;
	unsigned game_speed;
	unsigned difficulty;
	unsigned scroll_speed;
	unsigned flags;
	unsigned width, height;
	char pathfind, mp_pathfind;
};

extern struct config reg_cfg;

#endif
