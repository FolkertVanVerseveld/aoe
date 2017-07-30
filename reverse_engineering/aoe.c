/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Age of Empires
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 */
#include "cpu.h"
#include "dbg.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

#define GAME_TITLE "Age of Empires"
#define GAME_VERSION "00.09.12.1215"

#define PATH_DATA "data\\empires.dat"
#define PATH_REG_ROOT "Software\\Microsoft\\Games\\Age of Empires\\1.00"

static void some_main_this(void *this, const char *title, int a3)
{
	stub

	(void)this;
	(void)title;
	(void)a3;
}

static void some_main(void *this, const char *title, int ignored)
{
	(void)ignored;

	some_main_this(this, title, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	char str_title[101];
	char str_version[21];
	char str_title2[121];
	char str_tr_wrld[261];
	char path_data[261];
	char str_abbr[263];
	char path_reg_root[256];
	char str_cmdline[256];
	char str_icon[41];
	char some_str[41];
	char str_palette[261];
	char str_cursor[261];

	char dir_data[261];
	char dir_savegame[261];
	char dir_scenario[261];
	char dir_campaign[261];
	char dir_sound[261];
	char dir_data2[261];
	char dir_data3[261];
	char dir_avi[264];

	DWORD hidden10;
	DWORD hiddenC;
	DWORD hidden4;

	struct cpu cpu;

	stub

	(void)hInstance;
	(void)hPrevInstance;
	(void)nShowCmd;

	strcpy(str_title, GAME_TITLE);
	strcpy(str_version, GAME_VERSION);
	sprintf(str_title2, "%s", str_title);
	strcpy(str_tr_wrld, "tr_wrld.txt");
	strcpy(path_data, PATH_DATA);
	strcpy(path_reg_root, PATH_REG_ROOT);
	strcpy(str_cmdline, lpCmdLine);
	strcpy(str_icon, "AppIcon");
	strcpy(some_str, "");
	strcpy(str_palette, "palette");
	strcpy(str_cursor, "mcursors");
	strcpy(str_abbr, "AOE");

	strcpy(dir_data, "data\\");
	strcpy(dir_sound, "sound\\");
	strcpy(dir_savegame, "savegame\\");
	strcpy(dir_scenario, "scenario\\");
	strcpy(dir_campaign, "campaign\\");
	strcpy(dir_data2, "data\\");
	strcpy(dir_data3, "data\\");
	strcpy(dir_avi, "avi\\");

	/* exit(0x11A0); // XXX why */

	cpu.regs.e[REG_EAX] = 0x003A0020;
	cpu.regs.e[REG_EBX] = 0;

	hidden10 = cpu.regs.e[REG_EAX];
	hidden4 = cpu.regs.e[REG_EBX];

	(void)hiddenC;
	(void)hidden4;

	if (hidden10) {
		some_main(hidden10, str_title, 1);

		cpu.regs.e[REG_EDI] = cpu.regs.e[REG_EAX];
	}

	return 0;
}
