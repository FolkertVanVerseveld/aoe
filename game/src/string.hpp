#pragma once

#include <cstring>

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

}
