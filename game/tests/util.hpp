#pragma once

#include <cstddef>
#include <cstdarg>

#define FAIL(fmt, ...) fail(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define VFAIL(fmt, args) vfail(__FILE__, __LINE__, __func__, fmt, args)

namespace aoe {

void fail(const char *file, size_t lno, const char *func, const char *fmt, ...);
void vfail(const char *file, size_t lno, const char *func, const char *fmt, va_list args);

}
