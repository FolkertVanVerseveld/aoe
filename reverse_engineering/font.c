#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define CLASSNAME "ttfdump"

#define WIDTH 800
#define HEIGHT 600

HWND hwnd;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (msg) {
	case WM_DESTROY:
	case WM_CHAR:
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		{
		HFONT hfont;
		RECT rect;
		TEXTMETRIC fm; // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-textmetrica
		const char *fname = "Arial";
		int pt = 13;

		hdc = BeginPaint(hwnd, &ps);
			hfont = CreateFont(
				-pt, 0,
				0, 0,
				700, // 0=regular, 700=bold
				0, 0, 0,
				1, 0, 0, // TODO figure out what iCharSet=1 is
				NONANTIALIASED_QUALITY,//NONANTIALIASED_QUALITY, // antialiasing would be nice, but then we have to deal with transparency
				2,
				fname
			);

			// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gettextmetrics
			if (!GetTextMetrics(hdc, &fm)) {
				MessageBox(hwnd, "Text metrics unavailable", "Fatal Error", MB_ICONERROR | MB_OK);
				abort();
			}

			puts("font metrics:");
			printf("height                 : %ld\n", fm.tmHeight);
			printf("ascent                 : %ld\n", fm.tmAscent);
			printf("descent                : %ld\n", fm.tmDescent);
			printf("internal leading       : %ld\n", fm.tmInternalLeading);
			printf("external leading       : %ld\n", fm.tmExternalLeading);
			printf("average character width: %ld\n", fm.tmAveCharWidth);
			printf("maximum character width: %ld\n", fm.tmMaxCharWidth);
			printf("weight                 : %ld\n", fm.tmWeight);
			printf("overhang               : %ld\n", fm.tmOverhang);
			printf("digitized aspect x     : %ld\n", fm.tmDigitizedAspectX);
			printf("digitized aspect y     : %ld\n", fm.tmDigitizedAspectY);
			printf("first character        : %X\n", fm.tmFirstChar);
			printf("last character         : %X\n", fm.tmLastChar);
			printf("default character      : %X\n", fm.tmDefaultChar);
			printf("break character        : %X\n", fm.tmBreakChar);
			printf("italic                 : %X\n", fm.tmItalic);
			printf("underlined             : %X\n", fm.tmUnderlined);
			printf("struck out             : %X\n", fm.tmStruckOut);
			printf("pitch and family       : %X\n", fm.tmPitchAndFamily);
			printf("character set          : %X\n", fm.tmCharSet);

			SelectObject(hdc, hfont);
			{
				int glyph_start = fm.tmFirstChar;

				//SetBkMode(hdc, TRANSPARENT);

				SetBkColor(hdc, RGB(0xa0, 0xa0, 0xa0));
				SetTextColor(hdc, RGB(0xff, 0xff, 0xff));

				int glyph_width = 0, glyph_height = 0;

				// determine maximum glyph size
				for (int y = 0, glyph = 0; y < 16; ++y) {
					for (int x = 0; x < 16; ++x, ++glyph) {
						char text[2];
						SIZE dim;

						if (glyph < glyph_start)
							continue;

						text[0] = glyph;
						text[1] = 0;

						GetTextExtentPoint32A(hdc, text, 1, &dim);

						if (dim.cx > glyph_width)
							glyph_width = dim.cx;
						if (dim.cy > glyph_height)
							glyph_height = dim.cy;
					}
				}

				// draw font glyphes
				for (int y = 0, glyph = 0; y < 16; ++y) {
					for (int x = 0; x < 16; ++x, ++glyph) {
						char text[2];
						SIZE dim;

						if (glyph < glyph_start)
							continue;

						text[0] = glyph;
						text[1] = 0;

						int left, right, top, bottom;

						left = x * glyph_width;
						top = y * glyph_height;
						right = left + glyph_width;
						bottom = top + glyph_height;

						SetRect(&rect, left, top, right, bottom);
						DrawText(hdc, text, -1, &rect, DT_NOCLIP); // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawtext
						GetTextExtentPoint32A(hdc, text, 1, &dim);

						printf("%02X: %ld,%ld\n", glyph, dim.cx, dim.cy);
					}
				}
			}
			DeleteObject(hfont);
		EndPaint(hwnd, &ps);
		break;
		}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd)
{
	MSG msg;
	WNDCLASSEX window;

	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nShowCmd;

	ZeroMemory(&window, sizeof window);

	window.cbSize = sizeof window;
	window.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	window.lpfnWndProc = (WNDPROC)MainWndProc;
	window.cbClsExtra = 0;
	window.cbWndExtra = 0;
	window.hInstance = hInstance;
	window.hIcon = NULL;
	window.hCursor = LoadCursor(NULL, IDC_ARROW);
	window.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));//(HBRUSH)COLOR_WINDOW;
	window.lpszMenuName = NULL;
	window.lpszClassName = CLASSNAME;

	if (!RegisterClassEx(&window)) {
		MessageBox(NULL, "Could not register class", "Fatal error", MB_ICONERROR | MB_OK);
		return 1;
	}

	if (!(hwnd = CreateWindowEx(
			0, CLASSNAME, "Font dumper",
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			WIDTH, HEIGHT, HWND_DESKTOP, NULL, hInstance, NULL))) {
		MessageBox(NULL, "Could not create window", "Fatal error", MB_ICONERROR | MB_OK);
		return 1;
	}

	ShowWindow(hwnd, SW_SHOW);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
