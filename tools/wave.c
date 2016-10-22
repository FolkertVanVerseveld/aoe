/* AGPLv3 licensed */
/*
Simple wave inspector for data resource sets.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

static char *drs_sfx = NULL;
static struct stat st_sfx;

int main(int argc, char **argv)
{
	int ret = 1;
	FILE *f = NULL;
	if (argc != 2) {
		fprintf(stderr, "usage: %s path\n", argc > 0 ? argv[0] : "wave");
		goto fail;
	}
	const char *drs = argv[1];
	if (!(f = fopen(drs, "rb")) || stat(drs, &st_sfx)) {
		perror(drs);
		goto fail;
	}
	size_t n = st_sfx.st_size;
	printf("%s: %zu bytes\n", drs, n);
	if (!(drs_sfx = malloc(n)) || fread(drs_sfx, 1, n, f) != n) {
		perror(drs);
		goto fail;
	}
	fclose(f);
	f = NULL;
fail:
	if (drs_sfx)
		free(drs_sfx);
	if (f)
		fclose(f);
	return ret;
}
