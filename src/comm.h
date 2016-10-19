#ifndef AOE_COMM_H
#define AOE_COMM_H

#include "config.h"

struct comm14C4 {
	unsigned dword0;
	unsigned dword4;
	unsigned dword8;
	char gapC[264];
	void *ptr114;
};

struct comm_event {
	unsigned dword0;
	unsigned dword4;
	unsigned dword8;
	unsigned dwordC;
	unsigned dword10;
	unsigned dword14;
	unsigned dword18;
	unsigned dword1C;
};

struct comm_msg {
	char char0;
	char byte1;
	unsigned dword4;
	unsigned large_player_id;
};

struct comm_player {
	char gap0[568];
	unsigned dword238;
};

struct comm_v7 {
	void *pvoid0;
	unsigned dword4;
	unsigned dword8;
	unsigned dwordC;
	unsigned int num10;
	unsigned int num14;
	unsigned int num18;
};

struct comm {
	char char0;
	char gap1[3];
	struct game_settings *opt;
	unsigned num8;
	unsigned numC;
	char gap10[8];
	unsigned dword18;
	unsigned dword1C;
	char gap20[4005];
	char buf_version_match[259];
	unsigned timestamp10C8;
	char gap10CC[20];
	unsigned player_id10E0;
	unsigned dword10E4;
	unsigned dword10E8;
	unsigned dword10EC;
	float num10F0;
	float num10F4;
	char gap10F8[428];
	unsigned version;
	unsigned dword12A8;
	char gap12AC[20];
	unsigned dword12C0;
	char gap12C4[12];
	unsigned time12D0;
	char gap12D4[4];
	unsigned short player_id_max;
	short word12DA;
	unsigned int dword12DC;
	char gap12E0[44];
	// REMAP typeof(HWND hWnd130C) == unsigned
	unsigned hWnd130C;
	unsigned int tbl1310[44];
	unsigned int tbl13C0[65];
	struct comm14C4 *ptr;
	unsigned int tbl14C8[3];
	struct comm_player *comm_player14D4;
	unsigned dword14D8;
	char gap14DC[4];
	// REMAP typeof(HWND *pphwnd__14E0) == unsigned*
	unsigned *pphwnd__14E0;
	unsigned int large_player_id;
	char gap14E8[44];
	char byte1514;
	unsigned int dword1518;
	unsigned int dword151C;
	unsigned int dword1520;
	struct comm_v7 **comm_v7_1524;
	char gap1528[40];
	char byte1550;
	unsigned int num1554;
	unsigned int num1558;
	unsigned int timestamp158C;
	unsigned int num1560;
	unsigned int num1564;
	unsigned int player_data[10];
	char byte1590;
	char gap1591[233];
	unsigned tbl16D4[15];
	short word1710;
	short player_id_plus_1;
	char gap1714[4];
	unsigned int num1718;
	unsigned int opt_size;
	char gap1720[40];
	unsigned int num1748;
	char gap174C[4];
	unsigned int tbl1750[8];
};

extern struct comm *comm_580DA8;

int commhnd423D10(struct comm *this, unsigned int player_id);
int comm_no_msg_slot(struct comm *this);
int comm_opt_grow(struct comm *this, struct game_settings *opt, unsigned size);

#endif
