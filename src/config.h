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

struct color_cfg {
	int brightness;
	unsigned dword4;
	unsigned dword8;
	unsigned dwordC;
	unsigned dword10;
	unsigned dword14;
	char gap18[12];
	int int24;
	char gap28[50];
	char byte5A;
	char byte5B;
	char byte5C;
	char byte5D;
	char gap5E[2];
	unsigned dword60;
	unsigned dword64;
	char byte68;
	char byte69;
	char byte6A;
};

struct game_settings {
	float gamespeed;
	char c0;
	char str[128];
	char game_running;
	char num97E_97D_is_zero;
	char hsv[3];
	char cheats;
	char mp_pathfind;
	char ch8C;
	char ch8D;
	char ch8E;
	char ch8F;
	char ch90;
	char ch91;
	char player_tbl[9];
	char difficulty;
	union {
		char array[12];
		struct {
			unsigned num9C;
			unsigned numA0;
			unsigned numA4;
		} flat;
	} tbl;
	struct color_cfg colcfg;
};

void config_free(void);
int config_init(void);
int config_load(void);
int config_save(void);

#endif
