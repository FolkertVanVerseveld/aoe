#ifndef AOE_CONFIG_H
#define AOE_CONFIG_H

#define PLAYER_TBLSZ 9

#define PATHFIND_LOW 0
#define PATHFIND_MEDIUM 1
#define PATHFIND_HIGH 2

#define CFG_FULL 1

#define SCREEN_SIZE_MIN 400
#define SCREEN_SIZE_MAX 1920

#define ROLLOVER_TEXT_MIN 0
#define ROLLOVER_TEXT_MAX 2

#define MOUSE_STYLE_MIN 1
#define MOUSE_STYLE_MAX 2

#define CUSTOM_MOUSE_MIN 0
#define CUSTOM_MOUSE_MAX 2

#define SFX_VOLUME_MIN 0

#define GAME_SPEED_MIN 1

#define DIFFICULTY_MIN 0
#define DIFFICULTY_MAX 5

#define SCROLL_SPEED_MIN 1

#define PATHFIND_MIN 1
#define PATHFIND_MAX 127

#define MP_PATHFIND_MIN 1
#define MP_PATHFIND_MAX 127

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
	unsigned display;
	char *root_path;
	unsigned mode_fixed;
};

#define RES_LOW 0
#define RES_MED 1
#define RES_HIGH 2
#define RES_LARGE 3

#define COLOR_CFG_TBLSIZE PLAYER_TBLSZ

extern struct config reg_cfg;

struct color_cfg {
	int brightness;
	unsigned dword4;
	unsigned dword8;
	unsigned dwordC;
	unsigned dword10;
	char tbl14[COLOR_CFG_TBLSIZE];
	char gap19[3];
	int  tbl20[COLOR_CFG_TBLSIZE];
	char tbl44[COLOR_CFG_TBLSIZE];
	char tbl4D[COLOR_CFG_TBLSIZE];
	char ch56;
	char ch57;
	char ch58;
	char ch59;
	char byte5A;
	char byte5B;
	unsigned num5C;
	unsigned num60;
	char ch64;
	char ch65;
	char ch66;
	char ch67;
	char byte68;
	char byte69;
	char byte6A;
};

struct game_settings {
	float gamespeed;
	char c0;
	char str[128];
	char game_running;
	char game_stopped;
	char hsv[3];
	char cheats;
	char mp_pathfind;
	char byte8C;
	char byte8D;
	char byte8E;
	char byte8F;
	char byte90;
	char byte91;
	char player_tbl[PLAYER_TBLSZ];
	char difficulty;
	union {
		char array[12];
		struct {
			unsigned num9C;
			unsigned numA0;
			unsigned numA4;
			unsigned ptrA8;
		} flat;
	} tbl;
	struct color_cfg colcfg;
};

void config_free(void);
int config_init(void);
int config_load(char **path);
int config_save(void);

#endif
