#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include "../../genie/drs.h"
#include "../../src/dbg.h"
#include "../../src/dbg.c"

#define OPT_LIST 1
#define OPT_DUMP 2

static unsigned opts = 0;

static struct option long_opt[] = {
	{"help", no_argument, 0, 0},
	{0, 0, 0, 0},
};

static int walk(char *data, size_t size, size_t offset)
{
	struct drs_list *list = (struct drs_list*)(data + offset);
	if (list->offset > size) {
		fputs("bad list offset\n", stderr);
		return 1;
	}
	if (list->offset + list->size * sizeof(struct drs_item) > size) {
		fputs("corrupt list\n", stderr);
		return 1;
	}
	struct drs_item *item = (struct drs_item*)(data + list->offset);
	for (unsigned j = 0; j < list->size; ++j, ++item) {
		if (list->offset + (j + 1) * sizeof(struct drs_item) > size) {
			fputs("bad list\n", stderr);
			return 1;
		}
		const char *type = "?";
		switch (list->type) {
		case DT_WAVE  : type = "audio";  break;
		case DT_SHP   : type = "shape";  break;
		case DT_SLP   : type = "model";  break;
		case DT_BINARY: type = "object"; break;
		}
		if (opts & OPT_DUMP) {
			printf("%5u %8X %8X %8s ", item->id, item->offset, item->size, type);
			hexdump(data + item->offset, item->size > 16 ? 16 : item->size);
		} else
			printf("%5u %8X %8X %8s\n", item->id, item->offset, item->size, type);
	}
	return 0;
}

static int drsmap_stat(char *data, size_t size)
{
	if (size < sizeof(struct drsmap))
		goto bad_hdr;
	struct drsmap *drs = (struct drsmap*)data;
	if (strncmp("1.00tribe", drs->version, strlen("1.00tribe"))) {
bad_hdr:
		fputs("invalid drs file\n", stderr);
		return 1;
	}
	if (!drs->nlist) {
		puts("empty drs");
		return 0;
	}
	struct drs_list *list = (struct drs_list*)((char*)drs + sizeof(struct drsmap));
	unsigned nwave, nshp, nslp, nbin, nblob;
	nwave = nshp = nslp = nbin = 0;
	for (unsigned i = 0; i < drs->nlist; ++i, ++list) {
		switch (list->type) {
		case DT_WAVE:
			printf("audio: %u\n", list->size);
			nwave += list->size;
			break;
		case DT_SHP:
			printf("shapes: %u\n", list->size);
			nshp += list->size;
			break;
		case DT_SLP:
			printf("models: %u\n", list->size);
			nslp += list->size;
			break;
		case DT_BINARY:
			printf("objects: %u\n", list->size);
			nbin += list->size;
			break;
		default:
			printf("unknown: %u\n", list->size);
			nblob += list->size;
			break;
		}
	}
	list = (struct drs_list*)((char*)drs + sizeof(struct drsmap));
	if (opts & OPT_LIST)
		for (unsigned i = 0; i < drs->nlist; ++i, ++list)
			if (walk(data, size, (size_t)((char*)list - data)))
				return 1;
	return 0;
}

#define usage(f,n) fprintf(f, "usage: %s file\n", n)

static int parse_opt(int argc, char **argv)
{
	int c;
	while (1) {
		int option_index;
		c = getopt_long(argc, argv, "htd", long_opt, &option_index);
		if (c == -1) break;
		switch (c) {
		case 'h':
			usage(stdout, argc > 0 ? argv[0] : "view");
			break;
		case 't':
			opts |= OPT_LIST;
			break;
		case 'd':
			opts |= OPT_DUMP;
			break;
		default:
			fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}
	return optind;
}

static int process(const char *name)
{
	struct stat st;
	char *map = MAP_FAILED;
	size_t mapsz = 0;
	int fd, ret = 1;
	if ((fd = open(name, O_RDONLY)) == -1 || fstat(fd, &st)) {
		perror(name);
		goto fail;
	}
	map = mmap(NULL, mapsz = st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		perror(name);
		goto fail;
	}
	ret = drsmap_stat(map, mapsz);
fail:
	if (map != MAP_FAILED)
		munmap(map, mapsz);
	if (fd != -1)
		close(fd);
	return ret;
}

int main(int argc, char **argv)
{
	int argp;
	if (argc < 2) {
		usage(stderr, argc > 0 ? argv[0] : "view");
		return 1;
	}
	argp = parse_opt(argc, argv);
	for (; argp < argc; ++argp) {
		puts(argv[argp]);
		if (process(argv[argp]))
			return 1;
	}
	return 0;
}
