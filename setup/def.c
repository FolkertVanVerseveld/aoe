
/* Copyright 2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Common used routines
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 */

#include "def.h"

#include <stdio.h>
#include <stdlib.h>

#define PANIC_BUFSZ 1024

void panic(const char *str)
{
	char buf[PANIC_BUFSZ];
	snprintf(buf, PANIC_BUFSZ, "zenity --error --text=\"%s\"", str);
	system(buf);
	exit(1);
}
