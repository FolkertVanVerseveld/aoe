#ifndef _AOE_GAME_SET_H
#define _AOE_GAME_SET_H
/* only game.c may include this! */

static inline char *game_str8FD(struct game *this, char *str)
{
	char *ptr = strncpy(this->str8FD, str, 128);
	this->str8FD[127] = '\0';
	return ptr;
}

static inline void game_set8F8(struct game *this, float value)
{
	this->num8F8 = value;
}

static inline void game_set8FC(struct game *this, char value)
{
	this->ch8FC = value;
}

static inline void game_set9A0(struct game *this, unsigned value)
{
	this->num9A0 = value;
}

static inline void game_set9A4(struct game *this, unsigned value)
{
	this->num9A4 = value;
}

static inline int game_set97D_97E(struct game *this, unsigned value)
{
	this->num97D_97E_is_zero = value;
	return this->num97E_97D_is_zero = value == 0;
}

static inline int game_set97E_97D(struct game *this, unsigned value)
{
	this->num97E_97D_is_zero = value;
	return this->num97D_97E_is_zero = value == 0;
}

static inline char game_set982(struct game *this, char v)
{
	return this->ch982 = v;
}

static inline char game_set984(struct game *this, char v)
{
	return this->ch984 = v;
}

static inline char game_set985(struct game *this, char v)
{
	return this->ch985 = v;
}

static inline char game_set986(struct game *this, char v)
{
	return this->ch986 = v;
}

static inline char game_set987(struct game *this, char v)
{
	return this->ch987 = v;
}

static inline char game_set988(struct game *this, char v)
{
	return this->ch988 = v;
}

static inline char game_set989(struct game *this, char v)
{
	return this->ch989 = v;
}

static inline char game_set993(struct game *this, char v)
{
	return this->ch993 = v;
}

static inline char game_tbl98A(struct game *this, int index, char v)
{
	return this->tbl98A[index] = v;
}

static inline char game_tbl98A_2(struct game *this, int index, char mask)
{
	return this->tbl98A[index] = mask | (this->tbl98A[index] & 0xFE);
}

static inline char game_tbl98A_3(struct game *this, int index, char value)
{
	return this->tbl98A[index] = 2 * value | (this->tbl98A[index] & 1);
}

static inline char game_tbl994(struct game *this, int index, char v)
{
	return this->tbl994[index] = v;
}

static inline unsigned game_setA80(struct game *this, unsigned v)
{
	return this->numA80 = v;
}

static inline unsigned game_setA84(struct game *this, unsigned v)
{
	return this->numA84 = v;
}

static inline unsigned game_setA88(struct game *this, unsigned v)
{
	return this->numA88 = v;
}

static inline unsigned game_setA8C_A90(struct game *this, unsigned v1, unsigned v2)
{
	this->numA90 = v2;
	return this->numA8C = v1;
}

static inline char game_setAD6(struct game *this, char ch)
{
	return this->tblAD6[0] = ch;
}

static inline char game_setAD7(struct game *this, char ch)
{
	return this->tblAD6[1] = ch;
}

static inline char game_setAD8(struct game *this, char ch)
{
	return this->tblAD6[2] = ch;
}

static inline char game_setAD9(struct game *this, char ch)
{
	return this->tblAD6[3] = ch;
}

static inline char game_setADC(struct game *this, unsigned v)
{
	return this->numADC = v;
}

static inline char game_setAE0(struct game *this, unsigned v)
{
	return this->numAE0 = v;
}

static inline char game_chAE4(struct game *this, char ch)
{
	return this->chAE4 = ch;
}

static inline char game_chAE5(struct game *this, char ch)
{
	return this->chAE5 = ch;
}

static inline char game_chAE6(struct game *this, char ch)
{
	return this->chAE6 = ch;
}

static inline char game_ch1190(struct game *this, char ch)
{
	return this->ch1190 = ch;
}

static inline char game_set1194(struct game *this, int v)
{
	return this->num1194 = v;
}

static inline char game_ch988(struct game *this, char ch)
{
	return this->ch988 = ch;
}

static inline int game_clear1198(struct game *this)
{
	memset(this->blk1198, 0, 160);
	return 0;
}

static inline char game_offsetA94(struct game *this, int index, char ch)
{
	return this->tblA94[index] = ch;
}

static inline char game_offsetAC4(struct game *this, int index, char ch)
{
	return this->tblAC4[index] = ch;
}

static inline char game_offsetACD(struct game *this, int index, char ch)
{
	return this->tblACD[index] = ch;
}

#endif
