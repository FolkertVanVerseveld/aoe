#include <stdio.h>
#include <string.h>
#include "langx.h"

#define item(x,str) case x: res=str; break;

int loadstr(unsigned id, char *str, unsigned n)
{
	char *res = NULL;
	// TODO scan language files for UTF16 string
	if (!res) fprintf(stderr, "bad str: %u\n", id);
	else strncpy(str, res, n);
	return res != NULL;
}
