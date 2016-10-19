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

struct game_settings {
	float gamespeed;
	char byte4;
	char char5;
	char gap6[127];
	char ch85;
	char ch86;
	char ch87;
	char ch88;
	char ch89;
	char ch8A;
	char ch8B;
	char ch8C;
	char ch8D;
	char ch8E;
	char ch8F;
	char ch90;
	char ch91;
	char gap92[9];
	char byte9B;
	char gap9C[5];
	unsigned numA0;
	unsigned numA4;
};

void config_free(void);
int config_init(void);
int config_load(void);
int config_save(void);

#endif
