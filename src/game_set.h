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

static inline int game_settings_set97D_97E(struct game *this, int value)
{
	this->settings.num97D_97E_is_zero = value;
	return this->settings.num97E_97D_is_zero = value == 0;
}

static inline int game_settings_set97E_97D(struct game *this, int value)
{
	this->settings.num97E_97D_is_zero = value;
	return this->settings.num97D_97E_is_zero = value == 0;
}

static inline char game_settings_set8C(struct game *this, char value)
{
	return this->settings.ch8C = value;
}

static inline char game_settings_set8D(struct game *this, char value)
{
	return this->settings.ch8D = value;
}

static inline char game_settings_set8E(struct game *this, char value)
{
	return this->settings.ch8E = value;
}

static inline char game_settings_set8F(struct game *this, char value)
{
	return this->settings.ch8F = value;
}

static inline char game_settings_set90(struct game *this, char value)
{
	return this->settings.ch90 = value;
}

static inline char game_settings_set91(struct game *this, char value)
{
	return this->settings.ch91 = value;
}

static inline int game_set9A0(struct game *this, int value)
{
	return this->num9A0 = value;
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

static inline int game_setA80(struct game *this, int value)
{
	return this->numA80 = value;
}

static inline int game_setA84(struct game *this, int value)
{
	return this->numA84 = value;
}

static inline int game_setA88(struct game *this, int value)
{
	return this->numA88 = value;
}

static inline int game_setA8C_A90(struct game *this, int v0, int v1)
{
	this->numA8C = v0;
	this->numA90 = v1;
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

static int game_offsetAA0(struct game *this, int index, int value)
{
	assert(index < 12);
	return this->tblAA0[index] = value;
}

static char game_offsetA94(struct game *this, int index, char value)
{
	assert(index < 12);
	return this->tblA94[index] = value;
}

static char game_offsetAC4(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->tblAC4[index] = value;
}

static char game_offsetACD(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->tblACD[index] = value;
}

static inline int game_setAD6(struct game *this, int value)
{
	return this->chAD6 = value;
}

static inline int game_setAD7(struct game *this, int value)
{
	return this->chAD7 = value;
}

static inline int game_setAD8(struct game *this, int value)
{
	return this->chAD8 = value;
}

static inline int game_setAD9(struct game *this, int value)
{
	return this->chAD9 = value;
}

static inline int game_setADC(struct game *this, int value)
{
	return this->numADC = value;
}

static inline int game_setAE0(struct game *this, int value)
{
	return this->numAE0 = value;
}

static inline char game_setAE4(struct game *this, char value)
{
	return this->chAE4 = value;
}

static inline char game_setAE5(struct game *this, char value)
{
	return this->chAE5 = value;
}

static inline char game_setAE6(struct game *this, char value)
{
	return this->chAE6 = value;
}

static inline char game_set1190(struct game *this, char value)
{
	return this->ch1190 = value;
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
