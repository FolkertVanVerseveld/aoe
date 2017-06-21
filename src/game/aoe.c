/* Licensed under Affero General Public License v3.0
Copyright by Folkert van Verseveld et al. */

/**
 * Age of Empires - The Original and Rise of Rome Expansion
 *
 * Revised main program startup executable.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <genie/engine.h>

#define TITLE "Age of Empires - Free Software Remake"

int prng_seed;

int main(int argc, char *argv[])
{
	int argp;
	argp = ge_init(argc, argv, TITLE, GE_INIT_LEGACY_OPTIONS);
	return ge_main();
}
