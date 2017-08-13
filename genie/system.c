/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "system.h"
#include "dbg.h"

#include <stdio.h>
#include <stdlib.h>

int genie_system(const char *command)
{
	dbgs(command);
	return system(command);
}
