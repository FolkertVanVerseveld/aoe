#ifndef AOE_CONFIG_H
#define AOE_CONFIG_H

#define PATHFIND_LOW 0
#define PATHFIND_MEDIUM 1
#define PATHFIND_HIGH 2

struct config {
	unsigned screen_size;
	unsigned rollover_text;
	unsigned mouse_style;
	unsigned custom_mouse;
	unsigned sfx_volume;
	unsigned game_speed;
	unsigned difficulty;
	unsigned scroll_speed;
	unsigned flags;
	unsigned width, height;
	char pathfind, mp_pathfind;
};

extern struct config reg_cfg;

void config_free(void);
int config_init(void);
int config_load(void);
int config_save(void);

#endif
