#ifndef AOE_GAME_SET_H
#define AOE_GAME_SET_H

#include <assert.h>
#include <string.h>

static inline char game_settings_set_c0(struct game *this, char value)
{
	return this->settings.c0 = value;
}

static inline char *game_settings_set_str(struct game *this, const char *str)
{
	char *ptr = strncpy(this->settings.str, str, 128);
	this->settings.str[127] = '\0';
	return ptr;
}

static inline int game_settings_set_running(struct game *this, int value)
{
	this->settings.game_running = value;
	return this->settings.game_stopped = value == 0;
}

static inline int game_settings_set_stopped(struct game *this, int value)
{
	this->settings.game_stopped = value;
	return this->settings.game_running = value == 0;
}

static inline char game_settings_set8C(struct game *this, char value)
{
	return this->settings.byte8C = value;
}

static inline char game_settings_set8D(struct game *this, char value)
{
	return this->settings.byte8D = value;
}

static inline char game_settings_set8E(struct game *this, char value)
{
	return this->settings.byte8E = value;
}

static inline char game_settings_set8F(struct game *this, char value)
{
	return this->settings.byte8F = value;
}

static inline char game_settings_set90(struct game *this, char value)
{
	return this->settings.byte90 = value;
}

static inline char game_settings_set91(struct game *this, char value)
{
	return this->settings.byte91 = value;
}

static inline int game_settings_set_ptrA8(struct game *this, int value)
{
	return this->settings.tbl.flat.ptrA8 = value;
}

static inline int game_set9A4(struct game *this, int value)
{
	return this->num9A4 = value;
}

static inline char game_settings_tbl(struct game *this, int index, char value)
{
	assert(index < 12);
	return this->settings.tbl.array[index] = value;
}

static inline int game_set_color_cfg_brightness(struct game *this, int value)
{
	return this->col.brightness = value;
}

static inline int game_set_color_cfg_dword4(struct game *this, int value)
{
	return this->col.dword4 = value;
}

static inline int game_set_color_cfg_dword8(struct game *this, int value)
{
	return this->col.dword8 = value;
}

static inline int game_set_color_cfg_dwordC_dword10(struct game *this, int v0, int v1)
{
	this->col.dwordC  = v0;
	this->col.dword10 = v1;
	return v0;
}

static int game_offsetA24(struct game *this, int a2, int a3, int a4, int a5)
{
	this->tblA24[0] = a2;
	this->tblA24[1] = a3;
	this->tblA24[2] = a4;
	this->tblA24[3] = a5;
	return a4;
}

static int game_set_color_cfg_tbl20(struct game *this, int index, int value)
{
	assert(index < 9);
	return this->col.tbl20[index] = value;
}

static char game_set_color_cfg_tbl14(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->col.tbl14[index] = value;
}

static char game_set_color_cfg_tbl44(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->col.tbl44[index] = value;
}

static char game_set_color_cfg_tbl4D(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->col.tbl4D[index] = value;
}

static inline int game_set_color_cfg_ch56(struct game *this, int value)
{
	return this->col.ch56 = value;
}

static inline int game_set_color_cfg_ch57(struct game *this, int value)
{
	return this->col.ch57 = value;
}

static inline int game_set_color_cfg_ch58(struct game *this, int value)
{
	return this->col.ch58 = value;
}

static inline int game_set_color_cfg_ch59(struct game *this, int value)
{
	return this->col.ch59 = value;
}

static inline int game_set_color_cfg_num5C(struct game *this, int value)
{
	return this->col.num5C = value;
}

static inline int game_set_color_cfg_num60(struct game *this, int value)
{
	return this->col.num60 = value;
}

static inline int game_set_color_cfg_ch64(struct game *this, int value)
{
	return this->col.ch64 = value;
}

static inline int game_set_color_cfg_ch65(struct game *this, int value)
{
	return this->col.ch65 = value;
}

static inline int game_set_color_cfg_ch66(struct game *this, int value)
{
	return this->col.ch66 = value;
}

static inline char game_set1190(struct game *this, char value)
{
	return this->str1190[0] = value;
}

static inline int game_set1194(struct game *this, int value)
{
	return this->num1194 = value;
}

static inline int game_clear1198(struct game *this)
{
	memset(this->blk1198, 0, 160);
	return 0;
}

static inline char game_set_pathfind(struct game *this, char pathfind)
{
	return this->pathfind = pathfind;
}

static inline char game_settings_set_mp_pathfind(struct game *this, char mp_pathfind)
{
	return this->settings.mp_pathfind = mp_pathfind;
}

static inline char game_settings_set_hsv(struct game *this, unsigned char h, unsigned char s, unsigned char v)
{
	this->settings.hsv[0] = h;
	this->settings.hsv[1] = s;
	this->settings.hsv[2] = v;
	return v;
}

static inline char game_settings_set_cheats(struct game *this, char value)
{
	return this->settings.cheats = value;
}

static inline char game_settings_set_difficulty(struct game *this, char value)
{
	return this->settings.difficulty = value;
}

static inline char game_settings_init_player(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->settings.player_tbl[index] = value;
}

static inline char game_settings_player_ctl(struct game *this, int index, char mask)
{
	assert(index < 9);
	this->settings.player_tbl[index] = mask | (this->settings.player_tbl[index] & 0xFE);
	return index;
}

static inline char game_settings_player_ctl2(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->settings.player_tbl[index] = (2 * value) | (this->settings.player_tbl[index] & 1);
}

static inline int game_settings_player_is_alive(struct game *this, int player_id)
{
	assert(player_id < 9);
	return this->settings.player_tbl[player_id] & 1;
}

#endif
