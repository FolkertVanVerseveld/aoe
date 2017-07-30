/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Age of Empires CD setup
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

#define WINDOW_CLASS "AOESetupClass"

/* stringtable constants, they are sorted numerically */

#define STR_ERR_GAME_RUNNING 15
#define STR_ERR_OS_TOO_OLD 18
#define STR_ERR_FATAL 84

#define STR_CD_ROM_TITLE 95

#define STR_PATH_REGISTRY_UNINSTALL 200
#define STR_PATH_REGISTRY_SHARED_DLLS 201

#define STR_OPT_UNINSTALL 220
#define STR_OPT_NOSETUP 221
#define STR_ERR_SETUP 237
#define STR_OPT_AUTORUN 238

#define STR_COMMON_FILES_DIR 241
#define STR_PROGRAM_FILES_DIR 242
#define STR_PATH_REGISTRY_ROOT 306

#define STR_GAME_TITLE 311
#define STR_COMPLETE_GAME_TITLE 321
#define STR_GAME_TITLE2 335

#define STR_INSTALLATION_DIRECTORY 336
#define STR_INSTALLATION_ROOT_SUBDIRS 345

#define STR_SOME_NUMBER 364
#define STR_SOME_NUMBER2 365
#define STR_PATH_UNINSTALL 367

#define STR_PATCH "PATCH:"
#define REG_PATH_VERSION "Software\\Microsoft\\Windows\\CurrentVersion"

static struct cpu g_cpu;

static int dword_437E0C = 0;
static int dword_437F38 = 0;
static int main_singleton_required = 0;
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

static char path_registry_root[256];
static char game_title[256];
static char path_registry_uninstall[256];
static char path_registry_shared_dlls[256];
static char complete_game_title[136];

static int path_registry_root_length;

static int has_joysticks;

static const char *Default = "";

static HKEY hKey = NULL;

static int some_number = 0;
static int some_number2 = 0;

static char szLongPath[4096]; /* FIXME figure out length */

static int mach_type = 0;
static int mach_os_type = 0;
static int mach_w95_or_better = 0;

static int optbuf_length = 0;
static char optbuf[48];

static int opt_force = 0;
static int opt_autorun = 0;
static int opt_nosetup = 0;
static int opt_uninstall = 0;
static int some_path_skip_flag = 0;

static int some_sound_number = 0;

static HANDLE hObject = NULL;
static HDC hdc = NULL;
static HPALETTE hPal = NULL;

static int some_handle_msg2_tbl[1];
static int some_msg_event_number = 0;
static int some_msg_index = 0;
static char some_msg_mem[0x900]; /* FIXME figure out overlapping region */

#define IMG_LOAD_TABLE_SIZE 48

static int some_image_msg_tbl[6 * IMG_LOAD_TABLE_SIZE];
static int img_tbl[6 * IMG_LOAD_TABLE_SIZE];

static HANDLE img_lock = NULL;

#define SOME_MSG_MEM_OFFSET_PROC 0x3C

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

static LPCSTR interlocked_str(LPCSTR str)
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
	interlocked_str(String1);

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

/* Registry handling */

static LSTATUS reg_open_key_ex(HKEY hKey, LPCSTR name, DWORD options, REGSAM access, PHKEY retkey)
{
	dbgf("reg: open key \"%s\"\n", name);

	return RegOpenKeyExA(hKey, name, options, access, retkey);
}

static LSTATUS reg_query_key_ex(HKEY hkey, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count)
{
	dbgf("reg: query value \"%s\"\n", name);

	return RegQueryValueExA(hkey, name, reserved, type, data, count);
}

static LSTATUS reg_close_key(HKEY hkey)
{
	dbgf("reg: close key %X\n", (unsigned)hkey);

	return RegCloseKey(hkey);
}

static int reg_query_control_set(void)
{
	HKEY retkey;
	DWORD value = 0;
	DWORD n = 4;

	if (!reg_open_key_ex(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Windows", 0, 1, &retkey)) {
		reg_query_key_ex(retkey, "CSDVersion", 0, 0, (LPBYTE)&value, &n);
		reg_close_key(retkey);
	}

	return value >> 8;
}

/* other registry handling */

static int reg_str(HKEY root, const char *name, const char *str, const char *str2, char *buf, DWORD n)
{
	int n_copy;
	int result;

	n_copy = n;

	if (reg_open_key_ex(root, name, 0, 1, &hKey)) {
		lstrcpynA(buf, str2, n_copy);

		result = lstrlenA(str2);
	} else {
		if (reg_query_key_ex(hKey, str, NULL, 0, (LPBYTE)buf, &n)) {
			lstrcpynA(buf, str2, n_copy);
			n = lstrlenA(str2);
		}

		result = n;
	}

	printf("reg_str: str2=\"%s\"\n", str2);

	if (hKey)
		reg_close_key(hKey);

	return result;
}

static int reg_init_installation_path(const char *name, const char *str, char *dest, DWORD n)
{
	int n_copy;
	int result;

	HKEY retkey = NULL;

	n_copy = n;

	if (reg_open_key_ex(HKEY_LOCAL_MACHINE, path_registry_root, 0, 1, &retkey)) {
		lstrcpynA(dest, str, n_copy);

		result = lstrlenA(dest);
	} else {
		if (reg_query_key_ex(retkey, name, 0, 0, (LPBYTE)dest, &n)) {
			lstrcpynA(dest, str, n_copy);
			n = lstrlenA(str);
		}

		result = n;
	}

	printf("reg_init_installation_path: path=\"%s\"\n", dest);

	if (retkey)
		reg_close_key(retkey);

	return result;
}

static int init_str_files_dir(int use_common_dir, char *str, int n)
{
	int lang;

	char Buffer[25];
	char String2[512];
	char String1[512];

	lstrcpyA(String1, REG_PATH_VERSION);

	if (use_common_dir)
		LoadStringA(hInst, STR_COMMON_FILES_DIR, Buffer, sizeof Buffer);
	else
		LoadStringA(hInst, STR_PROGRAM_FILES_DIR, Buffer, sizeof Buffer);

	lang = GetSystemDefaultLangID() & 0x3ff;

	memset(String2, 0, sizeof String2);
	LoadStringA(hInst, lang + 400, String2, sizeof String2);

	return reg_str(HKEY_LOCAL_MACHINE, REG_PATH_VERSION, Buffer, String2, str, n);
}

static int init_str(void)
{
	char Buffer[50];
	char String1[260];
	char String2[260];

	/*
	 * NOTE some buffers are larger than the maximum requested data.
	 * it is either a programming error or intentional for other stuff.
	 */

	path_registry_root_length = LoadStringA(hInst,
		STR_GAME_TITLE, game_title, sizeof game_title
	);
	LoadStringA(hInst,
		STR_PATH_REGISTRY_UNINSTALL, path_registry_uninstall,
		sizeof path_registry_uninstall
	);
	LoadStringA(hInst,
		STR_PATH_REGISTRY_SHARED_DLLS, path_registry_shared_dlls,
		sizeof path_registry_shared_dlls
	);
	LoadStringA(hInst,
		STR_COMPLETE_GAME_TITLE, complete_game_title,
		sizeof complete_game_title
	);
	LoadStringA(hInst, STR_INSTALLATION_DIRECTORY, Buffer, sizeof Buffer);

	init_str_files_dir(0, String1, sizeof String1);

	lstrcatA(String1, "\\");
	LoadStringA(hInst, STR_INSTALLATION_ROOT_SUBDIRS, String2, sizeof String2);
	lstrcatA(String1, String2);

	reg_init_installation_path(Buffer, String1, szLongPath, sizeof String1);

	if (LoadStringA(hInst, STR_SOME_NUMBER, Buffer, sizeof Buffer) > 0)
		some_number = atol(Buffer);
	if (LoadStringA(hInst, STR_SOME_NUMBER2, Buffer, sizeof Buffer) > 0)
		some_number2 = atol(Buffer);

	has_joysticks = joyGetNumDevs() > 0;

	return 1;
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

static int get_os_info()
{
	int result;

	struct _OSVERSIONINFOA VersionInformation;

	if (mach_type)
		return mach_type;

	VersionInformation.dwOSVersionInfoSize = sizeof VersionInformation;
	GetVersionExA(&VersionInformation);

	if (!VersionInformation.dwPlatformId)
		return mach_type = 4;

	if (VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		mach_type = 8;

		if ((VersionInformation.dwBuildNumber & 0xffff) > 1000)
			mach_w95_or_better = 1;

		return mach_type;
	}

	if (VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT)
		return mach_type;

	mach_w95_or_better = 1;

	switch (VersionInformation.dwMajorVersion) {
	case 0:
	case 1:
	case 2:
	case 3: /* Windows NT 3.0 */
		return mach_type = 32;
	case 4:
		mach_type = 64;
		reg_query_control_set();
		return mach_type;
	case 5: /* Windows 2000 */
		return mach_type = 64;
	default:
		return mach_type;
	}

	return result;
}

static int optbuf_init(char *str)
{
	char *ptr;
	int state = 0;
	int state2 = 0;

	const char *path_end;
	const char *path_end_next;

	int result;

	char str_opt_nosetup[50];
	char Buffer[50];
	char str_opt_autorun[50];
	char str_opt_uninstall[50];
	char Filename[256];

	for (ptr = str; *ptr; ptr = CharNextA(ptr)) {
		int ch = *ptr;

		if (state2) {
			if (state) {
				if (ch == '"') {
					state = 0;
					state2 = 0;
					*ptr = '\0';
				}
			} else if (ch == ' ') {
				state2 = 0;
				*ptr = '\0';
			} else if (ch == '"') {
				state = 1;
				optbuf[optbuf_length + 0] = ptr[1];
				optbuf[optbuf_length + 1] = ptr[2];
				optbuf[optbuf_length + 2] = ptr[3];
				optbuf[optbuf_length + 3] = ptr[4];
				++optbuf_length;
			}
		} else {
			state2 = 1;

			if (ch == ' ') {
				optbuf[optbuf_length + 0] = ptr[1];
				optbuf[optbuf_length + 1] = ptr[2];
				optbuf[optbuf_length + 2] = ptr[3];
				optbuf[optbuf_length + 3] = ptr[4];
				++optbuf_length;
			} else {
				optbuf[optbuf_length + 0] = ptr[0];
				optbuf[optbuf_length + 1] = ptr[1];
				optbuf[optbuf_length + 2] = ptr[2];
				optbuf[optbuf_length + 3] = ptr[3];
				++optbuf_length;
			}
		}
	}

	optbuf_length = 0;

	GetModuleFileNameA(hmod, Filename, sizeof Filename);

	path_end = str_find_last_backslash(Filename);

	if (path_end)
		path_end_next = CharNextA(path_end);
	else
		path_end_next = Filename;

	LoadStringA(hInst, STR_PATH_UNINSTALL, Buffer, sizeof Buffer - 2);

	if (lstrcmpiA(path_end_next, Buffer) || main_singleton_required) {
		result = optbuf_length;

		if (optbuf_length > 0) {
			LoadStringA(hInst, STR_OPT_UNINSTALL, str_opt_uninstall, sizeof str_opt_uninstall);
			LoadStringA(hInst, STR_OPT_NOSETUP, str_opt_nosetup, sizeof str_opt_nosetup);
			LoadStringA(hInst, STR_OPT_AUTORUN, str_opt_autorun, sizeof str_opt_autorun);

			if (lstrcmpiA(optbuf, str_opt_uninstall)) {
				if (lstrcmpiA(optbuf, str_opt_nosetup)) {
					result = lstrcmpiA(optbuf, str_opt_nosetup);
					if (result) {
						result = lstrcmpiA(optbuf, str_opt_autorun);
						if (!result)
							opt_force = 1;
					} else
						opt_autorun = 1;
				} else
					opt_nosetup = 1;
			} else {
				result = 1;
				main_singleton_required = 1;
				opt_uninstall = 1;
			}
		}
	} else {
		result = 1;
		main_singleton_required = 1;
		opt_uninstall = 1;
		some_path_skip_flag = 1;
	}

	return result;
}

static int some_sound_cleanup(void)
{
	if (some_sound_number & 0x200000) {
		some_sound_number &= 0XFFDFFFFF;
		hObject = CreateEventA(0, 1, 0, 0);
		some_sound_number |= 0x100000;

		WaitForSingleObject(hObject, -1);
		CloseHandle(hObject);
	} else
		some_sound_number |= 0x100000;

	hObject = NULL;
	some_sound_number &= 0xFFF7FFFF;

	return GdiFlush();
}

static int some_message_wait(void)
{
	struct tagMSG Msg;

	if (PeekMessageA(&Msg, 0, 0, 0, 1)) {
		while (Msg.message != 18 && Msg.message != 16 && Msg.message != 2) {
			TranslateMessage(&Msg);
			DispatchMessageA(&Msg);

			if (!PeekMessageA(&Msg, 0, 0, 0, 1))
				return 1;
		}
		PostMessageA(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);

		return 0;
	} else
		return 1;
}

static int some_msg_event(int a1)
{
	stub

	(void)a1;

	return 0;
}

static int *msg_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

static int some_msg_event2(LONG dwNewLong)
{
	short *proc;
	int i;

	stub

	some_cursor_arg |= 0x40;
	if (some_msg_event_number) {
		/* function pointer black magic */
		if (dwNewLong == some_msg_event_number)
			some_cursor_arg &= 0xBF00;
		else
			some_cursor_arg |= 0x4000;
		if (IsWindow(ThreadId))
			SetFocus(ThreadId);
		if (!(some_handle_msg2_tbl[0] & 2)) {
			FIXME("some handle msg2 table loop");
		}
	}

	some_msg_index = (some_msg_index & ~0xFFFF) | -1;
	memset(&some_msg_mem, 0, sizeof some_msg_mem);

	for (proc = (short*)&some_msg_mem[SOME_MSG_MEM_OFFSET_PROC], i = 0; i < 32; ++i) {
		*proc = -1;
		proc += 36;
	}

	/* assert(dwNewLong == msg_proc); */
	FIXME("pointer black magic");
	int (*ptr)(HWND, UINT, WPARAM, LPARAM);

	ptr = (int (*)(HWND, UINT, WPARAM, LPARAM))msg_proc;

	ptr(NULL, 0, 0, 0);

	return 0;
}

static LRESULT CALLBACK handle_msg2(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	HPALETTE palette;
	HDC local_hdc;
	HCURSOR cursor;
	RECT rect;
	PAINTSTRUCT Paint;

	LPARAM lParamb;

	stub

	(void)hWnd;
	(void)wParam;
	(void)lParam;

	dbgf("%s: Msg == 0x%X\n", __func__, Msg);

	if (Msg <= WM_SETFOCUS) {
		if (Msg == WM_SETFOCUS) {
			FIXME("set focus");
		} else if (Msg == WM_DESTROY) {
			if (!main_singleton_required) {
				PlaySoundA(NULL, hmod, SND_PURGE);
				some_sound_cleanup();
			}
			PostQuitMessage(0);
			return 0;
		}
		goto propagate;
	}

	if (Msg <= WM_ERASEBKGND) {
		if (Msg != WM_ERASEBKGND) {
			if (Msg != WM_PAINT)
				return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
			if (!(some_cursor_arg & 0x40)) {
				BeginPaint(hWnd, &Paint);

				palette = SelectPalette(Paint.hdc, hPal, 0);
				RealizePalette(Paint.hdc);

				BitBlt(
					Paint.hdc,
					Paint.rcPaint.left,
					Paint.rcPaint.top,
					Paint.rcPaint.right - Paint.rcPaint.left + 1,
					Paint.rcPaint.bottom - Paint.rcPaint.top + 1,
					hdc,
					Paint.rcPaint.left,
					Paint.rcPaint.top,
					0xCC0020
				);

				EndPaint(hWnd, &Paint);
				GdiFlush();
			}
			return 0;
		}
		if (some_cursor_arg & 0x40)
			return 0;

		goto propagate;
	}

	if (Msg <= WM_DRAWITEM) {
		if (Msg != WM_DRAWITEM) {
			if (Msg == WM_SETCURSOR && (some_cursor_arg & 0x1000) == 0x1000)
				return 1;

			goto propagate;
		}
		if (!(some_cursor_arg & 0x40)) {
			FIXME("draw item");
		}
		return 0;
	}

	if (Msg <= WM_NCPAINT) {
		if (Msg != WM_NCPAINT) {
			if (Msg == WM_MEASUREITEM && *(DWORD*)lParam == 4)
				return 0;

			goto propagate;
		}
	}

	if (Msg <= WM_COMMAND) {
		if (Msg == WM_COMMAND) {
			if (wParam >> 16)
				goto propagate;

			FIXME("command");
		} else {
			if (Msg != WM_KEYFIRST)
				goto propagate;
			if (wParam == 9) {
				FIXME("async key focus");

				return 0;
			}
			if (wParam != 13 && wParam != 27)
				goto propagate;
			if (lParam & 0x40000000 || some_handle_msg2_tbl[0] & 2)
				return 0;

			FIXME("handle_msg2_tbl loop");
		}
	}

	if (Msg <= WM_INITMENU) {
		if (Msg == WM_INITMENU) {
			EnableMenuItem((HMENU)wParam, 0xF060, 1);
			return 0;
		}
		if (Msg != WM_SYSCOMMAND)
			goto propagate;

		if ((wParam & 0xFFF0) == 0xF170)
			return 0;
switch_sys_command:
		FIXME("switch sys command");

		goto propagate;
	}

	if (Msg <= WM_CTLCOLORSTATIC) {
		if (Msg == WM_CTLCOLORSTATIC) {
			FIXME("ctl color static");
		}
		if (Msg != WM_CTLCOLOREDIT)
			goto propagate;

		FIXME("ctl color edit");

		return GetStockObject(0);
	}

	if (Msg <= WM_POWERBROADCAST) {
		if (Msg != WM_POWERBROADCAST) {
			if (Msg == WM_LBUTTONDOWN && !hWndParent) {
				GetUpdateRect(hWnd, &rect, 0);
				RedrawWindow(hWnd, &rect, 0, 0x1A2);
				PostMessage(hWnd, WM_NCLBUTTONDOWN, 2, lParam);
			}
			goto propagate;
		}
		goto switch_sys_command;
	}

	if (Msg > 0x8B0) {
		if (Msg == 0x8CE) {
			Sleep(0);
			some_cursor_arg |= 0x10000;

			while (init_message_loop_running)
				some_message_wait();
		}
		init_message_loop_running = 1;

		cursor = LoadCursorA(NULL, IDC_WAIT);
		if (cursor) {
			some_cursor_arg &= 0xEF00;
			Sleep(0);
			SetCursor(cursor);
		}

		init_message_loop_running = 0;

		some_msg_event2(lParam);
		SendMessageA(ThreadId, 0x8EC, 0, 0);
		Sleep(0);
		some_cursor_arg &= 0xFFFEFFFF;
		return 0;
	} else {
		if (Msg != 0x8B0) {
			if (Msg != WM_QUERYNEWPALETTE && (Msg != WM_PALETTECHANGED || (HWND)wParam == hWnd))
				goto propagate;

			local_hdc = GetDC(hWnd);
			palette = SelectPalette(local_hdc, hPal, 0);
			lParamb = RealizePalette(local_hdc);
			SelectPalette(local_hdc, hPal, 1);
			RealizePalette(local_hdc);
			SelectPalette(local_hdc, palette, 0);
			ReleaseDC(hWnd, local_hdc);

			if (lParamb) {
				InvalidateRect(hWnd, 0, 0);
				return 1;
			}
			return 0;
		}
		if (some_msg_event(1) == 2)
			opt_uninstall = 0;

		PostMessage(ThreadId, 0x900, 0, 0);
		return 0;
	}

	return result;
propagate:
	return CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
}

#define PANIC(code, title) panic(__func__, code, title)

static inline void panic(const char *caller, DWORD last_error, LPCSTR title)
{
	HCURSOR cursor;

	int rem;
	int buflen;

	init_message_loop_running = 1;

	cursor = LoadCursorA(NULL, IDC_ARROW);
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
	fprintf(stderr, "%s: panic\n", caller);

	LoadStringA(hInst, STR_ERR_FATAL, Buffer, sizeof Buffer);

	rem = sizeof Buffer - lstrlenA(Buffer);
	buflen = lstrlenA(Buffer);

	FormatMessageA(0x1000, 0, last_error, 0x400, Buffer + buflen, rem, 0);

	if (IsWindow(ThreadId))
		FlashWindow(ThreadId, 1);

	MessageBeep(MB_ICONERROR);
	MessageBoxA(NULL, Buffer, title, MB_SETFOREGROUND | MB_ICONERROR);

	if (IsWindow(ThreadId))
		PostMessageA(ThreadId, 0x900, 0, 0);
}

static int init(HINSTANCE hinstance, LPSTR str, HWND a3, const char *lpClassName, unsigned short a5, int a6, int a7, WNDPROC a8, int a9)
{
	HWND hwnd;
	DWORD last_error;

	int os_good;
	DWORD build_number;

	HANDLE current_thread;
	char *name_pipe;
	const char *name_pipe_next;

	HWND main_window;
	HLOCAL cls_memlock;
	WNDCLASSA *cls;
	int atom;
	HWND win;
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

	dbgs("init_lang");

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

		PANIC(last_error, Caption);

		return 0;
	}

	dbgs("init_str");

	init_str();
	mach_os_type = get_os_info();

	VersionInformation.dwOSVersionInfoSize = sizeof VersionInformation;
	GetVersionExA(&VersionInformation);

	os_good = 1;

	if (mach_os_type & 8) {
		/* TODO some black magic */
		if (0 > (VersionInformation.dwBuildNumber & 0xffff))
			os_good = 0;
	} else if (mach_os_type & 0x40) {
		build_number = 1381; /* FIXME fetch this now precomputed value */

		if (build_number > VersionInformation.dwBuildNumber || (build_number == VersionInformation.dwBuildNumber && 0 > reg_query_control_set())) /* FIXME determine `0' expression */
			os_good = 0;
	} else
		os_good = 1;

	if (!os_good) {
		LoadStringA(hInst, STR_ERR_OS_TOO_OLD, Buffer, sizeof Buffer);
		MessageBoxA(0, Buffer, complete_game_title, MB_ICONWARNING);
		return 0;
	}

	current_thread = GetCurrentThread();
	SetThreadPriority(current_thread, 1);

	optbuf_init(str);

	LoadStringA(hInst, STR_GAME_TITLE2, Buffer, sizeof Buffer);
	name_pipe = strchr(Buffer, '|');
	if (name_pipe) {
		name_pipe_next = CharNextA(name_pipe);
		*name_pipe = '\0';
	} else
		name_pipe_next = NULL;

	main_window = FindWindowA(Buffer, name_pipe_next);
	if (main_window) {
		if (main_singleton_required) {
			LoadStringA(hInst, STR_ERR_GAME_RUNNING, Buffer, sizeof Buffer);
			MessageBoxA(NULL, Buffer, complete_game_title, MB_ICONWARNING);
		} else {
			if (IsIconic(main_window))
				ShowWindow(main_window, SW_RESTORE);
			SetForegroundWindow(main_window);
		}
		return 0;
	}

	cls_memlock = LocalAlloc(LMEM_ZEROINIT, sizeof *cls);
	if (!cls_memlock) {
		fprintf(stderr, "%s: alloc failed\n", __func__);

		last_error = GetLastError();

		while (init_message_loop_running)
			some_message_wait();

		goto err_alloc;
	}

	cls = LocalLock(cls_memlock);
	cls->style = 0x4000;
	cls->lpfnWndProc = handle_msg2;
	cls->hInstance = hinstance;
	cls->hIcon = LoadIconA(hmod, (LPCSTR)a5);
	cls->hCursor = LoadCursorA(0, IDC_ARROW);
	cls->hbrBackground = 0;
	cls->lpszMenuName = 0;
	cls->lpszClassName = lpClassName;

	atom = RegisterClass(cls);
	LocalUnlock(cls_memlock);
	LocalFree(cls_memlock);

	if (!atom) {
		fprintf(stderr, "%s: class failed\n", __func__);

		last_error = GetLastError();

		while (init_message_loop_running)
			some_message_wait();

err_alloc:
		PANIC(last_error, complete_game_title);

		return 0;
	}

	if (!LoadStringA(hInst, STR_CD_ROM_TITLE, Buffer, sizeof Buffer))
		lstrcpyA(Buffer, complete_game_title);

	dbgs("creating window...");

	win = CreateWindowExA(
		0, lpClassName, Buffer,
		0x82080000,
		0x80000000,
		0x80000000,
		0x80000000,
		0x80000000,
		hWndParent,
		0, hInstance, 0
	);

	if (!win) {
		fprintf(stderr, "%s: window failed\n", __func__);

		last_error = GetLastError();

		while (init_message_loop_running)
			some_message_wait();

		PANIC(last_error, complete_game_title);

		return 0;
	}

	ThreadId = win;

	/* ... */

	return 1;
}

static HMODULE __cdecl cleanup(LPCSTR lpClassName)
{
	stub

	(void)lpClassName;

	if (!main_singleton_required) {
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

static int *img_load(HINSTANCE hInst, int a2, int a3, int a4);

static int *msg_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	stub

	(void)hWnd;
	(void)Msg;
	(void)wParam;
	(void)lParam;

	some_sound_cleanup();

	img_load(hInst, 209, 0, 4);
	img_load(hInst, 209, 0, 4);
	img_load(hInst, 209, 0, 4);
	img_load(hInst, 209, 0, 4);
	img_load(hInst, 209, 0, 4);

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

static int *img_load(HINSTANCE hInst, int id, int a3, int a4)
{
	int v4;
	int *v5 = NULL;
	int v6;
	HANDLE img = NULL;

	stub

	(void)hInst;
	(void)a3;
	(void)a4;

	v4 = id;

	/* check if already loaded */
	for (v6 = 0; v6 < IMG_LOAD_TABLE_SIZE; ++v6) {
		if (id == some_image_msg_tbl[6 * v6]) {
			v5 = &some_image_msg_tbl[24 * v6];
			break;
		}
	}
	/* load if not loaded yet */
	if (!v5) {
		img = LoadImageA(hInst, id, 0, 0, 0, 0xA000);
		if (!img)
			return 0;

		WaitForSingleObject(img_lock, -1);
		for (v6 = 0; v6 < IMG_LOAD_TABLE_SIZE; ++v6) {
			if (!some_image_msg_tbl[6 * v6] || id == some_image_msg_tbl[6 * v6]) {
				img_tbl[6 * v6] = (int)img;
				break;
			}
		}
		ReleaseMutex(img_lock);
	}

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
		if (!main_singleton_required) {
			some_loop(IDC_WAIT);
			img_load(hInst, 209, 0, 4);

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
