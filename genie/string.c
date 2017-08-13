/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "string.h"

#include <ctype.h>
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

char *strncpy0(char *dest, const char *src, size_t n)
{
	char *ptr;
	ptr = strncpy(dest, src, n);
	if (n)
		dest[n - 1] = '\0';
	return ptr;
}

int strsta(const char *haystack, const char *needle)
{
	return strstr(haystack, needle) == haystack;
}

int parse_address(const char *str, uint64_t *address, uint64_t *mask)
{
	const char *base_str = "0123456789ABCDEF";
	unsigned base = 10;
	if (!*str)
		return 1;
	const char *ptr = str;
	uint64_t v = 0;
	switch (*ptr) {
	case '%': base =  2; ++ptr; break;
	case 'o': base =  8; ++ptr; break;
	case '$': base = 16; ++ptr; break;
	}
	*mask = 0;
	for (; *ptr; ++ptr) {
		v *= base;
		*mask *= base;
		const char *pos = strchr(base_str, toupper(*ptr));
		if (!pos || pos >= base_str + base)
			return 1;
		v += (unsigned)(pos - base_str);
		*mask += base - 1;
	}
	*address = v;
	return 0;
}

void strupper(char *str)
{
	for (unsigned char *ptr = (unsigned char*)str; *ptr; ++ptr)
		*ptr = toupper(*ptr);
}

void strlower(char *str)
{
	for (unsigned char *ptr = (unsigned char*)str; *ptr; ++ptr)
		*ptr = tolower(*ptr);
}
