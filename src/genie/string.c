#include "string.h"
#include <string.h>
#include "memmap.h"

void strmerge(char **dest, const char *str1, const char *str2)
{
	char *merge;
	size_t n1, n2;
	n1 = strlen(str1);
	n2 = strlen(str2);
	merge = new(n1 + n2);
	strncpy(merge, str1, n1);
	strncpy(&merge[n1], str2, n2);
#ifndef STRICT
	merge[n1 + n2 - 1] = '\0';
#endif
	if (*dest)
		delete(*dest);
	*dest = merge;
}
