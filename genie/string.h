/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef GENIE_STRING_H
#define GENIE_STRING_H

#include <stddef.h>
#include <stdint.h>

void strmerge(char **dest, const char *str1, const char *str2);
char *strncpy0(char *dest, const char *src, size_t n);
int strsta(const char *haystack, const char *needle);
int parse_address(const char *str, uint64_t *address, uint64_t *mask);

#endif
