/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

/**
 * Quick and dirty macros to determine target OS platform
 * Only linux and windows are supported at the moment, apple or some other unix-based OS may or may not work.
 */

#if defined(__gnu_linux__) || defined(linux) || defined(__linux__)
	#ifndef linux
		#define linux 1
	#endif
	#define OSNAME "linux"
#endif

#if _WIN32
	#define windows 1
#endif

#if _WIN64
	#define windows 1
#endif

#if windows
	#define OSNAME "windows"
#endif

#ifndef OSNAME
	#error Unknown target platform
#endif
