#pragma once

#include <cstring>
#include <string>

namespace aoe {

static char *strncpy0(char *dst, const char *src, size_t count)
{
#pragma warning(disable: 4996)
	char *ptr = strncpy(dst, src, count);
#pragma warning(default: 4996)
	if (count)
		dst[count - 1] = '\0';

	return ptr;
}

static void cstrncpy(std::string &dst, const char *src, size_t count)
{
	for (size_t i = 0; i < count; ++i) {
		if (src[i])
			dst.push_back(src[i]);
		else
			break;
	}
}

static size_t strmaxlen(const char *str, size_t count)
{
	size_t size = 0;

	while (*str++ && size < count)
		++size;

	return size;
}

}
