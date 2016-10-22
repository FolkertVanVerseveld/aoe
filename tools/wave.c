/* licensed under Affero General Public License version 3 */
/*
Simple wave inspector for data resource sets.
*/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "drs.h"

#define WAVE_CHUNK_ID 0x46464952

int drsmap_stat(char *data, size_t size)
{
	if (size < sizeof(struct drsmap)) {
		fputs("invalid drs file\n", stderr);
		return 1;
	}
	struct drsmap *drs = (struct drsmap*)data;
	if (strncmp("1.00tribe", drs->version, strlen("1.00tribe"))) {
		fputs("invalid drs file\n", stderr);
		return 1;
	}
	if (!drs->nlist)
		return 0;
	struct drs_list *list = (struct drs_list*)((char*)drs + sizeof(struct drsmap));
	for (unsigned i = 0; i < drs->nlist; ++i, ++list) {
		if (list->type != DT_WAVE)
			continue;
		// resource offset or something
		if (list->offset > size) {
			fputs("bad item offset\n", stderr);
			return 1;
		}
		printf("items: %u\n", list->size);
		struct drs_item *item = (struct drs_item*)(data + list->offset);
		for (unsigned j = 0; j < list->size; ++j, ++item) {
			if (list->offset + (j + 1) * sizeof(struct drs_item) > size) {
				fputs("bad list\n", stderr);
				return 1;
			}
			uint32_t *magic = (uint32_t*)(data + item->offset);
			if (*magic != WAVE_CHUNK_ID) {
				fprintf(stderr, "bad wave at %x: magic=%x\n", item->offset, *magic);
				return 1;
			}
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 1;
	FILE *f = NULL;
	char *drs_sfx = NULL;
	struct stat st_sfx;
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
	ret = drsmap_stat(drs_sfx, (size_t)st_sfx.st_size);
fail:
	if (drs_sfx)
		free(drs_sfx);
	if (f)
		fclose(f);
	return ret;
}
