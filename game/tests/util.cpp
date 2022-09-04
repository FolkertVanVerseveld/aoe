#include "util.hpp"

#include <cstdio>

namespace aoe {

void fail(const char *file, size_t lno, const char *func, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfail(file, lno, func, fmt, args);
	va_end(args);
}

void vfail(const char *file, size_t lno, const char *func, const char *fmt, va_list args)
{
	fprintf(stderr, "%s:%llu:%s: ", file, (unsigned long long)lno, func);
	vfprintf(stderr, fmt, args);
}

}
