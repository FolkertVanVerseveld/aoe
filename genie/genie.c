/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <genie/genie.h>

#include <stdio.h>
#include <string.h>

int GE_Init(int *pargc, char **argv)
{
	int argc = *pargc;

	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];

		if (!strcmp(arg, "--version") || !strcmp(arg, "-v")) {
			puts("Genie Engine v0");
		}

		if (!strcmp(arg, "--"))
			break;
	}

	return 0;
}

int GE_Quit(void)
{
	return 0;
}
