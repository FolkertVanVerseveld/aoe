/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Age of Empires CD setup
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 */
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define stub fprintf(stderr, "stub: %s\n", __func__);

#define WINDOW_CLASS "AOESetupClass"

static struct cpu g_cpu;

static int dword_437E0C = 0;
static int dword_437F38 = 0;
static int dword_4392E8 = 0;
static int chdir_patch = 0;

static WNDPROC lpPrevWndFunc = NULL;
static HINSTANCE hInstance = NULL;
static HINSTANCE hmod = NULL;
static HWND hWndParent = NULL;
static HWND ThreadId = NULL;

static char PathName[260];
static char Caption[136];

static const char *Default = "";

static void g_init(void)
{
	memset(&g_cpu, 0, sizeof g_cpu);
}

static char *strncpy0(char *dest, const char *src, size_t n)
{
	char *result;

	result = strncpy(dest, src, n);

	if (n)
		dest[n - 1] = '\0';

	return result;
}

#define STR_PATCH "PATCH:"

#define LOCK_ID

static LPCSTR sub_42E630(LPCSTR str)
{
	LPCSTR result;

	stub

	result = str;

	return result;
}

static const char *str_find_last_backslash(LPCSTR str)
{
	const char *v1;
	const char *v2;
	int ch;

	for (v1 = str, v2 = NULL, ch = *str; *v1; ch = *v1) {
		if (ch == '\\')
			v2 = v1;

		v1 = CharNextA(v1);
	}

	return v2;
}

static int check_patch(LPCSTR str)
{
	char *v1;
	int v2;
	char *path_split;
	int split_length;

	const char *path_end;
	const char *path_end_next;

	char str_patch[260];
	char FileName[260];
	char String1[260];
	char FileName2[260];
	char Text[1024];

	strncpy0(str_patch, STR_PATCH, sizeof str_patch);

	lstrcpyA(String1, str);
	sub_42E630(String1);

	v1 = strstr(String1, "PATCH:");

	if (v1) {
		v2 = lstrlenA(str_patch);

		lstrcpyA((LPSTR)&PathName, &v1[v2]);

		for (path_split = PathName; *path_split; ++path_split)
			if (*path_split == ' ')
				break;

		if (*path_split)
			*path_split = '\0';

		split_length = lstrlenA(PathName);

		if (*CharPrevA(PathName, &PathName[split_length]) != '\\')
			lstrcatA(PathName, "\\");

		GetModuleFileNameA(hmod, FileName2, sizeof FileName2);

		path_end = str_find_last_backslash(FileName2);
		path_end_next = CharNextA(path_end);

		lstrcpyA(FileName, PathName);
		lstrcatA(FileName, path_end_next);

		if (GetFileAttributesA(FileName) == INVALID_FILE_ATTRIBUTES) {
			sprintf(Text, "Setup being patched not found at %s unable to continue", FileName);

			MessageBoxA(0, Text, Caption, 0x30);

			return 0;
		}

		chdir_patch = 1;
		SetCurrentDirectoryA(PathName);
	}

	return 1;
}

static int sub_40DCD0(int size, const char *str, int a3)
{
	LCID lcid;
	char LibFileName[256];

	stub

	(void)size;
	(void)a3;
	(void)lcid;

	if (check_patch(str)) {
		lcid = GetUserDefaultLCID();

		strncpy0(LibFileName, Default, sizeof LibFileName);
	}

	return 0;
}

static int init(HINSTANCE hInst, LPSTR str, HWND a3, const char *lpClassName, unsigned short a5, int a6, int a7, WNDPROC a8)
{
	HWND v9;

	stub

	(void)a5;
	(void)a6;

	dword_437E0C = a7;
	lpPrevWndFunc = a8;
	hInstance = hInst;
	hmod = hInstance;
	hWndParent = a3;

	v9 = FindWindowA(lpClassName, 0);

	if (v9) {
		SetForegroundWindow(v9);
		return 0;
	}

	if (!sub_40DCD0(1024, str, 0)) {
	}

	return 1;
}

static HMODULE __cdecl cleanup(LPCSTR lpClassName)
{
	stub

	(void)lpClassName;

	if (!dword_4392E8) {
		if (IsWindow(ThreadId))
			SetFocus(ThreadId);

		if (!(dword_437F38 & 2)) {
		}
	}

	return 0;
}

static LRESULT CALLBACK handle_msg(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	int v5;
	int v6;

	stub

	(void)v5;
	(void)v6;

	switch (Msg) {
	case 0x8BA:
		v5 = wParam;

		result = 0;

		break;
	case 0x8D8:
		result = 0;
		break;
	case 0x8E2:
		result = 0;
		break;
	default:
		return DefWindowProcA(hWnd, Msg, wParam, lParam);
	}

	return result;
}

static int *msg_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	stub

	(void)hWnd;
	(void)Msg;
	(void)wParam;
	(void)lParam;

	return NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	struct cpu cpu;

	int result;

	struct tagMSG Msg;

	char v6;
	int v7;
	int v8;
	char v9;
	short v10;

	stub

	(void)hPrevInstance;
	(void)nShowCmd;

	(void)v6;
	(void)v7;
	(void)v8;
	(void)v9;
	(void)v10;

	g_init();

	memset(&cpu, 0, sizeof cpu);
	memset(&Msg, 0, sizeof Msg);

	v8 = 1381;
	v9 = 3;
	v10 = 0;
	v7 = 8;

	result = init(
		hInstance, lpCmdLine, 0,
		WINDOW_CLASS,
		1028, 162, 1, handle_msg
	);

	if (result) {
		if (!dword_4392E8) {
			SetForegroundWindow(ThreadId);
			SendMessageA(ThreadId, /* IDM_APPLYNORMAL */ 0x8CE, 0, (LPARAM)msg_proc);
		}

		while (GetMessageA(&Msg, 0, 0, 0)) {
			TranslateMessage(&Msg);
			DispatchMessageA(&Msg);
		}

		cleanup(WINDOW_CLASS);

		result = Msg.wParam;
	}

	return 0;
}
