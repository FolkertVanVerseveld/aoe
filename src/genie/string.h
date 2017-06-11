#ifndef GENIE_STRING_H
#define GENIE_STRING_H

#include <stddef.h>

void strmerge(char **dest, const char *str1, const char *str2);
char *strncpy0(char *dest, const char *src, size_t n);

#endif
