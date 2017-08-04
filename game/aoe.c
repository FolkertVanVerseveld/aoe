/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Age of Empires - The Original and Rise of Rome Expansion
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Revised main program startup executable.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <genie/dmap.h>
#include <genie/engine.h>

#define TITLE "Age of Empires - Free Software Remake"

int prng_seed;

struct dmap drs_data_list[] = {
	{.filename = DRS_DATA DRS_SFX, .nommap = 1},
	{.filename = DRS_DATA DRS_GFX},
	{.filename = DRS_DATA DRS_MAP},
	{.filename = DRS_DATA DRS_BORDER},
	{.filename = DRS_DATA DRS_UI},
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

int main(int argc, char *argv[])
{
	int argp;

	(void)argp;

	argp = genie_init(argc, argv, TITLE, GENIE_INIT_LEGACY_OPTIONS);
	dmap_set_list(drs_data_list, ARRAY_SIZE(drs_data_list));

	return genie_main();
}
