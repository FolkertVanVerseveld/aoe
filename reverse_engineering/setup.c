/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Age of Empires CD setup
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 */
#include "cpu.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#ifdef DEBUG
#include <stdio.h>
#define dbgs(s) puts(s)
#define dbgf(f,...) printf(f,##__VA_ARGS__)
#else
#define dbgs(s) ((void)0)
#define dbgf(f,...) ((void)0)
#endif

#define stub fprintf(stderr, "stub: %s\n", __func__);

#define WINDOW_CLASS "AOESetupClass"

#define STR_ERR_FATAL 84
#define STR_ERR_SETUP 237

#define STR_PATCH "PATCH:"

static struct cpu g_cpu;

static int dword_437E0C = 0;
static int dword_437F38 = 0;
static int some_main_skip_flag = 0;
static int init_message_loop_running = 0;
static int chdir_patch = 0;
static int setup_data_binary = 0;
static int (*init_lang_msg_box)(HWND, LPCTSTR, LPCTSTR, UINT) = NULL;

static int some_cursor_arg = 0;

static WNDPROC lpPrevWndFunc = NULL;
static HINSTANCE hInstance = NULL;
static HINSTANCE hmod = NULL;
static HWND hWndParent = NULL;
static HWND ThreadId = NULL;
static HMODULE hInst = NULL;
static HMODULE hModule = NULL;

static char PathName[260];
static char Caption[136];

static char Buffer[1024];

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

static HMODULE load_library(LPCSTR libname)
{
	printf("load lib: %s\n", libname);
	return LoadLibrary(libname);
}

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

			MessageBoxA(0, Text, Caption, MB_ICONWARNING);

			return 0;
		}

		chdir_patch = 1;
		SetCurrentDirectoryA(PathName);
	}

	return 1;
}

static int alert(HWND hWnd, UINT uType, UINT uID, ...)
{
	UINT utype;
	int result;

	char Buffer[1024];
	char Text[2048];

	va_list args;

	va_start(args, uID);

	strncpy0(Buffer, "DEFAULT BUFFER", sizeof Buffer);
	strncpy0(Text, "NO STRING LOADED", sizeof Text);

	SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

	if (LoadStringA(hInst, uID, Buffer, sizeof Buffer))
		wvsprintfA(Text, Buffer, args);
	else
		sprintf(Text, "Alert(): Error loading string resource # %d.", uID);

	utype = uType;

	if (!hWnd)
		uType |= MB_TASKMODAL;

	if (init_lang_msg_box)
		result = init_lang_msg_box(hWnd, Text, Caption, utype);
	else
		result = MessageBoxA(hWnd, Text, Caption, utype);

	va_end(args);

	return result;
}

static int init_lang(int size, const char *str, int (*msg_box_func)(HWND, LPCTSTR, LPCTSTR, UINT))
{
	LCID lcid;
	HMODULE mod;
	char v7[4];
	char LCData[4];
	char LibFileName[256];

	(void)size; /* even unused in dissassembly */

	if (check_patch(str)) {
		if (msg_box_func)
			init_lang_msg_box = msg_box_func;

		strncpy0(LibFileName, Default, sizeof LibFileName);

		lcid = GetUserDefaultLCID();

		GetLocaleInfoA(lcid, 3, LCData, sizeof LCData);
		sprintf(LibFileName, "%s%s", "Setup", LCData);

		mod = load_library(LibFileName);
		hInst = mod;

		if ((int)mod < 32) {
			GetLocaleInfoA((lcid & 0x3FF) | 0x400, 3, v7, sizeof v7);

			sprintf(LibFileName, "%s%s", "Setup", v7);

			mod = load_library(LibFileName);
			hInst = mod;

			if ((int)mod < 32) {
				mod = load_library("SetupENU.dll");
				hInst = mod;

				if ((int)mod < 32) {
					MessageBoxA(0, "Unable to find the language specific dll unable to continue.", "Setup", 0);

					return 0;
				}
			}
		}

		if (FindResourceA(mod, "SETUPDATA", "SETUPBINARY")) {
			setup_data_binary = 1;
			hModule = hInst;

			return 1;
		}

		if (FindResourceA(hmod, "SETUPDATA", "SETUPBINARY")) {
			setup_data_binary = 1;
			hModule = hmod;

			return 1;
		}

		if (FindResourceA(hInst, (LPCSTR)0x65, "SETUPINFO")) {
			hModule = hInst;

			return 1;
		}

		if (FindResourceA(hmod, (LPCSTR)0x65, "SETUPINFO")) {
			hModule = hmod;

			return 1;
		}

		alert(NULL, MB_ICONERROR, STR_ERR_SETUP);
	}

	return 0;
}

static int init(HINSTANCE hinstance, LPSTR str, HWND a3, const char *lpClassName, unsigned short a5, int a6, int a7, WNDPROC a8, int a9)
{
	HWND hwnd;
	DWORD last_error;

	HCURSOR cursor;

	int v14;
	int v15;

	struct tagMSG Msg;
	struct _OSVERSIONINFOA VersionInformation;

	stub

	(void)a5;
	(void)a6;
	(void)a9;

	dword_437E0C = a7;
	lpPrevWndFunc = a8;
	hInstance = hinstance;
	hmod = hinstance;
	hWndParent = a3;

	hwnd = FindWindowA(lpClassName, 0);

	if (hwnd) {
		SetForegroundWindow(hwnd);
		return 0;
	}

	if (!init_lang(1024, str, NULL)) {
		last_error = GetLastError();

		while (init_message_loop_running) {
			if (PeekMessageA(&Msg, 0, 0, 0, 1)) {
				while (Msg.message != 18 && Msg.message != 16 && Msg.message != 2) {
					TranslateMessage(&Msg);
					DispatchMessageA(&Msg);

					if (!PeekMessageA(&Msg, 0, 0, 0, 1))
						goto skip_post;
				}

				PostMessageA(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);
			}
skip_post:
			;
		}

		init_message_loop_running = 1;

		cursor = LoadCursorA(0, IDC_ARROW);

		if (cursor) {
			some_cursor_arg &= 0xef00;

			Sleep(0);
			SetCursor(cursor);
		}

		init_message_loop_running = 0;

		/**
		 * Inform user that fatal non-recoverable error occurred.
		 *
		 * Finally, halt and catch fire.
		 */
		fprintf(stderr, "%s: panic\n", __func__);

		LoadStringA(hInst, STR_ERR_FATAL, Buffer, sizeof Buffer);

		v14 = (sizeof Buffer) - lstrlenA(Buffer);
		v15 = lstrlenA(Buffer);

		FormatMessageA(0x1000, 0, last_error, 0x400, Buffer + v15, v14, 0);

		if (IsWindow(ThreadId))
			FlashWindow(ThreadId, 1);

		MessageBeep(MB_ICONERROR);
		MessageBoxA(0, Buffer, Caption, MB_SETFOREGROUND | MB_ICONERROR);

		if (IsWindow(ThreadId))
			goto post_some_msg;

		return 0;
	}

	dbgs("init lang");

	/*
	TODO port

	sub_4169D0();
	word_438C5C = sub_40D8A0();
	*/

	VersionInformation.dwOSVersionInfoSize = sizeof VersionInformation;
	GetVersionExA(&VersionInformation);

	if (0) {
post_some_msg:
		PostMessageA(ThreadId, 0x900, 0, 0);
		return 0;
	}

	return 1;
}

static HMODULE __cdecl cleanup(LPCSTR lpClassName)
{
	stub

	(void)lpClassName;

	if (!some_main_skip_flag) {
		if (IsWindow(ThreadId))
			SetFocus(ThreadId);

		if (!(dword_437F38 & 2)) {
		}
	}

	return 0;
}

static int some_cursor_arg_apply(int mask, int bitwise_or)
{
	int result;

	if (bitwise_or) {
		result = mask;
		some_cursor_arg |= mask;
	} else {
		result = ~mask & some_cursor_arg;
		some_cursor_arg &= ~mask;
	}

	return result;
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

static HCURSOR some_loop(LPCTSTR cursor_name)
{
	HCURSOR result;
	HCURSOR v2;

	struct tagMSG Msg;

	while (init_message_loop_running) {
		if (PeekMessageA(&Msg, 0, 0, 0, 1)) {
			while (Msg.message != 18 && Msg.message != 16 && Msg.message != 2) {
				TranslateMessage(&Msg);
				DispatchMessageA(&Msg);

				if (PeekMessageA(&Msg, 0, 0, 0, 1))
					goto skip_post;
			}

			PostMessageA(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);
		}
skip_post:
		;
	}

	init_message_loop_running = 1;

	result = LoadCursorA(0, cursor_name);
	v2 = result;

	if (result) {
		some_cursor_arg &= 0xEF00;

		Sleep(0);
		result = SetCursor(v2);
	}

	init_message_loop_running = 0;
	return result;
}

static int *some_image_ctl(HINSTANCE hInst, int a2, int a3, int a4)
{
	int v4;
	int *v5 = NULL;

	stub

	(void)hInst;
	(void)a3;
	(void)a4;

	v4 = a2;

	(void)v4;

	return v5;
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

	(void)hPrevInstance;
	(void)nShowCmd;

	(void)v7;
	(void)v8;
	(void)v9;
	(void)v10;

	g_init();

	memset(&cpu, 0, sizeof cpu);
	memset(&Msg, 0, sizeof Msg);

	/* XXX v7, v8, v9 and v10 may be arguments to init */
	v8 = 1381;
	v9 = 3;
	v10 = 0;
	v7 = 8;

	result = init(
		hInstance, lpCmdLine, 0,
		WINDOW_CLASS,
		1028, 162, 1, handle_msg, (int)&v6
	);

	if (result) {
		if (!some_main_skip_flag) {
			some_loop(IDC_WAIT);
			some_image_ctl(hInst, 209, 0, 4);

			SetForegroundWindow(ThreadId);
			SendMessageA(ThreadId, /* IDM_APPLYNORMAL */ 0x8CE, 0, (LPARAM)msg_proc);

			some_loop(IDC_ARROW);
			some_cursor_arg_apply(0x400, 1);
		}

		while (GetMessageA(&Msg, 0, 0, 0)) {
			TranslateMessage(&Msg);
			DispatchMessageA(&Msg);
		}

		cleanup(WINDOW_CLASS);

		result = Msg.wParam;
	}

	return result;
}
