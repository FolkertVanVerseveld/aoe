#pragma once

#include <time.h>

#if _WIN32
#include <Windows.h>
#include <stdint.h> // portable: uint64_t   MSVC: __int64 

// Sleep is in ms, sleep in secs
#define sleep(s) Sleep(s*1000)

#define CLOCK_MONOTONIC 0

// START code taken from https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
// windows version of posix clock_gettime. NOTE clock type is ignored
#define exp7           10000000i64     //1E+7     //C-file part
#define exp9         1000000000i64     //1E+9
#define w2ux 116444736000000000i64     //1.jan1601 to 1.jan1970

#if __cplusplus
extern "C" {
#endif

static void unix_time(struct timespec *spec)
{
	__int64 wintime; GetSystemTimeAsFileTime((FILETIME *)&wintime);
	wintime -= w2ux;  spec->tv_sec = wintime / exp7;
	spec->tv_nsec = wintime % exp7 * 100;
}

static int clock_gettime(int dummy, struct timespec *spec)
{
	(void)dummy;
	static  struct timespec startspec; static double ticks2nano;
	static __int64 startticks, tps = 0;    __int64 tmp, curticks;
	QueryPerformanceFrequency((LARGE_INTEGER *)&tmp); //some strange system can
	if (tps != tmp) {
		tps = tmp; //init ~~ONCE         //possibly change freq ?
		QueryPerformanceCounter((LARGE_INTEGER *)&startticks);
		unix_time(&startspec); ticks2nano = (double)exp9 / tps;
	}
	QueryPerformanceCounter((LARGE_INTEGER *)&curticks); curticks -= startticks;
	spec->tv_sec = startspec.tv_sec + (curticks / tps);
	spec->tv_nsec = startspec.tv_nsec + (double)(curticks % tps) * ticks2nano;
	if (!(spec->tv_nsec < exp9)) { spec->tv_sec++; spec->tv_nsec -= exp9; }
	return 0;
}
// END code taken from https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
#endif

static double elapsed_time(const struct timespec *start, const struct timespec *end)
{
	return (double)end->tv_sec - start->tv_sec + (end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

#if __cplusplus
}
#endif
