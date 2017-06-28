/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Simple window creation and message handling
 *
 * Licensed under the GNU Affero General Public License version 3
 * Copyright 2017 Folkert van Verseveld
 *
 * It is completely written from scratch without consulting the dissassembly.
 */
#include <windows.h>
#include <stdio.h>

static LPCTSTR WndName = "Dummy Window";
static LPCTSTR ClsName = "WindowApp";

static LRESULT CALLBACK handle_msg(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
	switch (m) {
	case WM_DESTROY:
		PostQuitMessage(WM_QUIT);
		break;
	default:
		return DefWindowProc(w, m, wp, lp);
	}
	return 0;
}

static int main_loop(void)
{
	MSG Msg;

	while (GetMessage(&Msg, NULL, 0, 0)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX cls;
	int x = 32, y = 32, width = 1024, height = 768;

	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;

	cls.cbSize        = sizeof cls;
	cls.style         = CS_HREDRAW | CS_VREDRAW;
	cls.lpfnWndProc   = handle_msg;
	cls.cbClsExtra    = 0;
	cls.cbWndExtra    = 0;
	cls.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	cls.hCursor       = LoadCursor(NULL, IDC_ARROW);
	cls.hbrBackground = GetStockObject(WHITE_BRUSH);
	cls.lpszMenuName  = NULL;
	cls.lpszClassName = ClsName;
	cls.hInstance     = hInstance;
	cls.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

	RegisterClassEx(&cls);

	hWnd = CreateWindow(
		ClsName, WndName,
		WS_OVERLAPPEDWINDOW,
		x, y, width, height,
		NULL, NULL, hInstance, NULL
	);

	if (!hWnd) {
		fputs("Could not create window\n", stderr);
		return 1;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	return main_loop();
}
