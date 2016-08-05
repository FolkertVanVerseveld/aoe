#ifndef AOE_GAME_SET_H
#define AOE_GAME_SET_H

#include <assert.h>
#include <string.h>

static inline char game_set988(struct game *this, char value)
{
	return this->ch988 = value;
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

#endif
