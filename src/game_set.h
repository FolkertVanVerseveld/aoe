#ifndef AOE_GAME_SET_H
#define AOE_GAME_SET_H

#include <assert.h>
#include <string.h>

static inline char game_set_c0(struct game *this, char value)
{
	return this->c0 = value;
}

static inline char *game_str_8FD(struct game *this, const char *str)
{
	char *ptr = strncpy(this->str8FD, str, 128);
	this->str8FD[127] = '\0';
	return ptr;
}

static inline int game_set97D_97E(struct game *this, int value)
{
	this->num97D_97E_is_zero = value;
	return this->num97E_97D_is_zero = value == 0;
}

static inline int game_set97E_97D(struct game *this, int value)
{
	this->num97E_97D_is_zero = value;
	return this->num97D_97E_is_zero = value == 0;
}

static inline char game_set984(struct game *this, char value)
{
	return this->ch984 = value;
}

static inline char game_set985(struct game *this, char value)
{
	return this->ch985 = value;
}

static inline char game_set986(struct game *this, char value)
{
	return this->ch986 = value;
}

static inline char game_set987(struct game *this, char value)
{
	return this->ch987 = value;
}

static inline char game_set988(struct game *this, char value)
{
	return this->ch988 = value;
}

static inline char game_set989(struct game *this, char value)
{
	return this->ch989 = value;
}

static inline int game_set9A0(struct game *this, int value)
{
	return this->num9A0 = value;
}

static inline int game_set9A4(struct game *this, int value)
{
	return this->num9A4 = value;
}

static inline char game_tbl994(struct game *this, int index, char value)
{
	assert(index < 12);
	return this->tbl994[index] = value;
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

static inline char game_set_mp_pathfind(struct game *this, char mp_pathfind)
{
	return this->mp_pathfind = mp_pathfind;
}

static inline char game_set_hsv(struct game *this, unsigned char h, unsigned char s, unsigned char v)
{
	this->hsv[0] = h;
	this->hsv[1] = s;
	this->hsv[2] = v;
	return v;
}

static inline char game_set_cheats(struct game *this, char value)
{
	return this->cheats = value;
}

static inline char game_set_difficulty2(struct game *this, char value)
{
	return this->difficulty2 = value;
}

static inline char game_init_player(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->player_tbl[index] = value;
}

static inline char game_player_ctl(struct game *this, int index, char mask)
{
	assert(index < 9);
	this->player_tbl[index] = mask | (this->player_tbl[index] & 0xFE);
	return index;
}

static inline char game_player_ctl2(struct game *this, int index, char value)
{
	assert(index < 9);
	return this->player_tbl[index] = (2 * value) | (this->player_tbl[index] & 1);
}

static inline int game_player_is_alive(struct game *this, int player_id)
{
	assert(player_id < 9);
	return this->player_tbl[player_id] & 1;
}

#endif
