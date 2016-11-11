/*
Proper codingstyle fixed by Folkert van Verseveld.
 _________
/         \ tinyfiledialogs.h v2.6.2 [November 6, 2016] zlib licence
|tiny file| Unique header file of "tiny file dialogs" created [November 9, 2014]
| dialogs | Copyright (c) 2014 - 2016 Guillaume Vareille http://ysengrin.com
\____  ___/ http://tinyfiledialogs.sourceforge.net
     \|           	                     mailto:tinyfiledialogs@ysengrin.com

A big thank you to Don Heyse http://ldglite.sf.net for
                   his code contributions, bug corrections & thorough testing!

            git://git.code.sf.net/p/tinyfiledialogs/code

Please
	1) let me know
	- if you are including tiny file dialogs,
	  I'll be happy to add your link to the list of projects using it.
	- If you are using it on different hardware / OS / compiler.
	2) Be the first to leave a review on Sourceforge. Thanks.

tiny file dialogs (cross-platform C C++)
InputBox PasswordBox MessageBox ColorPicker
OpenFileDialog SaveFileDialog SelectFolderDialog
Native dialog library for WINDOWS MAC OSX GTK+ QT CONSOLE & more

A single C file (add it to your C or C++ project) with 6 boxes:
- message / question
- input / password
- save file
- open file & multiple files
- select folder
- color picker.

Complements OpenGL GLFW GLUT GLUI VTK SFML SDL Ogre Unity ION
CEGUI MathGL CPW GLOW IMGUI GLT NGL STB & GUI less programs

NO INIT
NO MAIN LOOP

The dialogs can be forced into console mode

Windows [MBCS + UTF-8 + UTF-16]
- native code & some vbs create the graphic dialogs
- enhanced console mode can use dialog.exe from
http://andrear.altervista.org/home/cdialog.php
- basic console input

Unix [UTF-8] (command line call attempts)
- applescript
- zenity / matedialog
- kdialog
- Xdialog
- python2 tkinter
- dialog (opens a console if needed)
- basic console input
The same executable can run across desktops & distributions

tested with C & C++ compilers
on VisualStudio MinGW Mac Linux Bsd Solaris Minix Raspbian C# fortran (iso_c)
using Gnome Kde Enlightenment Mate Cinnamon Unity
Lxde Lxqt Xfce WindowMaker IceWm Cde Jds OpenBox

- License -

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software.  If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef TINYFILEDIALOGS_H
#define TINYFILEDIALOGS_H

#define TINYFD_QUERY "tinyfd_query"

/* #define TINYFD_NOLIB */
/* On windows, define TINYFD_NOLIB here
if you don't want to include the code creating the graphic dialogs.
Then you won't need to link against Comdlg32.lib and Ole32.lib */

/* if tinydialogs.c is compiled with a C++ compiler rather than with a C compiler
(ie. you change the extension from .c to .cpp), you need to comment out:
extern "C" {
and the corresponding closing bracket near the end of this file:
}
*/
#ifdef	__cplusplus
extern "C" {
#endif

extern char tinyfd_version[8]; /* contains tinyfd current version number */

#ifdef _WIN32
/* for UTF-16 use the functions at the end of this files */
extern int tinyfd_winUtf8; /* 0 (default) or 1 */
/* on windows string char can be 0:MBSC or 1:UTF-8
unless your code is really prepared for UTF-8 on windows, leave this on MBSC. */
#endif

extern int tinyfd_forceConsole;  /* 0 (default) or 1 */
/* for unix & windows: 0 (graphic mode) or 1 (console mode).
0: try to use a graphic solution, if it fails then it uses console mode.
1: forces all dialogs into console mode even when an X server is present,
  if the package dialog (and a console is present) or dialog.exe is installed.
  on windows it only make sense for console applications */

extern char tinyfd_response[1024];

/* if you pass "tinyfd_query" as aTitle,
the functions will not display the dialogs
but will return 0 for console mode, 1 for graphic mode.
tinyfd_response is then filled with the retain solution.
possible values for tinyfd_response are (all lowercase)
for the graphic mode:
  windows applescript zenity zenity3 matedialog kdialog
  xdialog tkinter gdialog gxmessage xmessage
for the console mode:
  dialog whiptail basicinput */

/* Display simple message box. returns 0 for cancel/no, 1 for ok/yes. */
int tinyfd_messageBox(
	const char *aTitle, /* "" */
	const char *aMessage, /* "" may contain \n \t */
	const char *aDialogType, /* "ok" "okcancel" "yesno" */
	const char *aIconType, /* "info" "warning" "error" "question" */
	int aDefaultButton /* 0 for cancel/no, 1 for ok/yes */
);

/* Ask user for input. returns NULL on cancel */
char *tinyfd_inputBox (
	const char *aTitle, /* "" */
	const char *aMessage, /* "" may NOT contain \n \t on windows */
	const char *aDefaultInput  /* "", if NULL it's a passwordBox */
);

/* Ask user for file to save to. returns NULL on cancel */
char *tinyfd_saveFileDialog (
	const char *aTitle, /* "" */
	const char *aDefaultPathAndFile, /* "" */
	int aNumOfFilterPatterns, /* 0 */
	const char **aFilterPatterns, /* NULL | {"*.jpg","*.png"} */
	const char *aSingleFilterDescription /* NULL | "text files" */
);

/*
Ask user for file to load from. returns NULL on cancel.
in case of multiple files, the separator is |
*/
char *tinyfd_openFileDialog(
	const char *aTitle, /* "" */
	const char *aDefaultPathAndFile, /* "" */
	int aNumOfFilterPatterns, /* 0 */
	const char **aFilterPatterns, /* NULL {"*.jpg","*.png"} */
	const char *aSingleFilterDescription, /* NULL | "image files" */
	int aAllowMultipleSelects /* 0 or 1 */
);

/* Ask user for directory. returns NULL on cancel. */
char *tinyfd_selectFolderDialog(
	const char *aTitle, /* "" */
	const char *aDefaultPath /* "" */
);

/*
Ask user for color. returns the hexcolor as a string "#FF0000".
aoResultRGB also contains the result.
aDefaultRGB is used only if aDefaultHexRGB is NULL.
aDefaultRGB and aoResultRGB can be the same array.
returns NULL on cancel
*/
char *tinyfd_colorChooser(
	const char *aTitle, /* "" */
	const char *aDefaultHexRGB, /* NULL or "#FF0000" */
	unsigned char const aDefaultRGB[3], /* {0, 255, 255} */
	unsigned char aoResultRGB[3] /* {0, 0, 0} */
);

/************ NOT CROSS PLATFORM SECTION STARTS HERE ************************/
#ifdef _WIN32
#ifndef TINYFD_NOLIB

/*
windows only - utf-16 version
returns 0 for cancel/no, 1 for ok/yes
*/
int tinyfd_messageBoxW(
	wchar_t const *aTitle,
	wchar_t const *aMessage, /* "" may contain \n \t */
	wchar_t const *aDialogType, /* "ok" "okcancel" "yesno" */
	wchar_t const *aIconType, /* "info" "warning" "error" "question" */
	int const aDefaultButton /* 0 for cancel/no, 1 for ok/yes */
);

/*
windows only - utf-16 version.
returns NULL on cancel
*/
wchar_t *tinyfd_saveFileDialogW(
	const wchar_t *aTitle, /* NULL or "" */
	const wchar_t *aDefaultPathAndFile, /* NULL or "" */
	int const aNumOfFilterPatterns, /* 0 */
	const wchar_t **aFilterPatterns, /* NULL or {"*.jpg","*.png"} */
	const wchar_t *aSingleFilterDescription /* NULL or "image files" */
);

/*
windows only - utf-16 version
in case of multiple files, the separator is |
returns NULL on cancel
*/
wchar_t *tinyfd_openFileDialogW(
	const wchar_t *aTitle, /* "" */
	const wchar_t *aDefaultPathAndFile, /* "" */
	int const aNumOfFilterPatterns, /* 0 */
	const wchar_t **aFilterPatterns, /* NULL {"*.jpg","*.png"} */
	const wchar_t *aSingleFilterDescription, /* NULL | "image files" */
	int const aAllowMultipleSelects /* 0 or 1 */
);

/*
windows only - utf-16 version
returns NULL on cancel
*/
wchar_t *tinyfd_selectFolderDialogW(
	const wchar_t *aTitle, /* "" */
	const wchar_t *aDefaultPath /* "" */
);

/*
windows only - utf-16 version
returns the hexcolor as a string "#FF0000".
aoResultRGB also contains the result.
aDefaultRGB is used only if aDefaultHexRGB is NULL.
aDefaultRGB and aoResultRGB can be the same array. returns NULL on cancel.
*/
wchar_t *tinyfd_colorChooserW(
	const wchar_t *aTitle, /* "" */
	const wchar_t *aDefaultHexRGB, /* NULL or "#FF0000" */
	unsigned char const aDefaultRGB[3], /* {0, 255, 255} */
	unsigned char aoResultRGB[3] /* {0, 0, 0} */
);

#endif /*TINYFD_NOLIB*/
#else /*_WIN32*/

/* unix zenity only */
char *tinyfd_arrayDialog(
	const char *aTitle, /* "" */
	int aNumOfColumns, /* 2 */
	const char **aColumns, /* {"Column 1","Column 2"} */
	int aNumOfRows, /* 2*/
	const char **aCells
);
/* {"Row1 Col1","Row1 Col2","Row2 Col1","Row2 Col2"} */

#endif /*_WIN32 */

#ifdef	__cplusplus
}
#endif

#endif /* TINYFILEDIALOGS_H */
