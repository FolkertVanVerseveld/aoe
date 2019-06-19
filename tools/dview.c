/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <ctype.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <alloca.h>
#include "../empires/drs.h"

void hexdump(const void *buf, size_t n) {
	uint16_t i, j, p = 0;
	const unsigned char *data = buf;
	while (n) {
		printf("%04hX", p & ~0xf);
		for (j = p, i = 0; n && i < 0x10; ++i, --n)
			printf(" %02hhX", data[p++]);
		putchar(' ');
		for (i = j; i < p; ++i)
			putchar(isprint(data[i]) ? data[i] : '.');
		putchar('\n');
	}
}

#define OPT_LIST 1
#define OPT_DUMP 2
#define OPT_UNPACK 4
#define OPT_UNPACKX 8
#define OPT_PACK 16

static unsigned opts = 0;
static unsigned unpackxid = 0;
static const char *packname = NULL;

static struct option long_opt[] = {
	{"help", no_argument, 0, 'h'},
	{"dump", no_argument, 0, 'd'},
	{0, 0, 0, 0},
};

static void usage(FILE *f, const char *name)
{
	fprintf(
		f, "usage: %s OPTIONS files\n"
		"options:\n"
		"--help -h  this help\n"
		"-t         list item ID, offset and size\n"
		"--dump -d  dump first 16 bytes of each file\n"
		"-x[ID]     extract all files or (just ID) from file\n"
		"-c         create archive (first file is destination)\n",
		name
	);
}

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
		case DT_WAVE  : type =  "audio"; break;
		case DT_SHP   : type =  "shape"; break;
		case DT_SLP   : type =  "model"; break;
		case DT_BINARY: type = "object"; break;
		}
		if (opts & OPT_DUMP) {
			printf("%5u %8X %8X %8s ", item->id, item->offset, item->size, type);
			if (item->offset + 16 > size) {
				fprintf(stderr, "corrupt item: %X, %zX\n", item->offset, size);
				return 1;
			}
			hexdump(data + item->offset, item->size > 16 ? 16 : item->size);
		} else {
			printf("%5u %8X %8X %8s\n", item->id, item->offset, item->size, type);
		}
	}
	return 0;
}

static int unpack(char *data, size_t size, size_t offset)
{
	char buf[16];
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
		if ((opts & OPT_UNPACKX) && item->id != unpackxid)
			continue;
		const char *type = "bin";
		switch (list->type) {
		case DT_WAVE: type = "wav"; break;
		case DT_SHP : type = "shp"; break;
		case DT_SLP : type = "slp"; break;
		}
		snprintf(buf, sizeof buf, "%05u.%s", item->id, type);
		FILE *f = fopen(buf, "wb");
		if (!f) goto fail;
		if (fwrite(data + item->offset, 1, item->size, f) != item->size) {
			fclose(f);
fail:
			perror(buf);
			return 1;
		}
		fclose(f);
	}
	return 0;
}

static int drs_stat(char *data, size_t size)
{
	if (size < sizeof(struct drs_hdr))
		goto bad_hdr;
	struct drs_hdr *drs = (struct drs_hdr*)data;
	if (strncmp("1.00tribe", drs->version, strlen("1.00tribe"))) {
bad_hdr:
		fputs("invalid drs file\n", stderr);
		return 1;
	}
	if (!drs->nlist) {
		puts("empty drs");
		return 0;
	}
	struct drs_list *list = (struct drs_list*)((char*)drs + sizeof(struct drs_hdr));
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
	uint32_t end = sizeof(struct drs_hdr) + drs->nlist * sizeof(struct drs_list);
	list = (struct drs_list*)((char*)drs + sizeof(struct drs_hdr));
	for (unsigned i = 0; i < drs->nlist; ++i, ++list)
		end += list->size * sizeof(struct drs_item);
	printf("listend: %u (0x%X)\n", drs->listend, drs->listend);
	printf("computed: %u (0x%X)\n", end, end);
	list = (struct drs_list*)((char*)drs + sizeof(struct drs_hdr));
	size_t off = (size_t)((char*)list - data);
	size_t offp = off;
	if (opts & OPT_UNPACK)
		for (unsigned i = 0; i < drs->nlist; ++i, ++list, off += sizeof(struct drs_list))
			if (unpack(data, size, off))
				return 1;
	off = offp;
	if (opts & OPT_LIST)
		for (unsigned i = 0; i < drs->nlist; ++i, ++list, off += sizeof(struct drs_list))
			if (walk(data, size, off))
				return 1;
	return 0;
}

static int parse_opt(int argc, char **argv)
{
	int c;
	while (1) {
		int option_index;
		c = getopt_long(argc, argv, "htdx::c:", long_opt, &option_index);
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
		case 'x':
			if (!optarg)
				opts |= OPT_UNPACK;
			else if (sscanf(optarg, "%u", &unpackxid) != 1) {
				fprintf(stderr, "bad unpack id: %s\n", optarg);
				return -1;
			} else {
				opts |= OPT_UNPACK | OPT_UNPACKX;
				printf("unpackxid: %u\n", unpackxid);
			}
			break;
		case 'c':
			opts |= OPT_PACK;
			packname = optarg;
			break;
		default:
			fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}
	if ((opts & OPT_PACK) && (opts & OPT_UNPACK)) {
		fputs("cannot pack and unpack at the same time\n", stderr);
		return -1;
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
	ret = drs_stat(map, mapsz);
fail:
	if (map != MAP_FAILED)
		munmap(map, mapsz);
	if (fd != -1)
		close(fd);
	return ret;
}

static int packlist(FILE *f, int type, off_t *pos, unsigned n)
{
	if (!n) return 1;
	struct drs_list list;
	list.type = type;
	list.offset = *pos;
	list.size = n;
	*pos += n * sizeof(struct drs_item);
	return fwrite(&list, sizeof list, 1, f);
}

static int packtbl(FILE *f, size_t rpos, unsigned n, unsigned type, char **argv, int argp, int argc, unsigned *itbl, unsigned *ntbl, size_t *sztbl)
{
	if (!n) return 0;
	struct drs_item item;
	for (int i = argp, j = 0; i < argc; ++i, ++j) {
		if (itbl[j] == type) {
			item.id = ntbl[j];
			item.offset = rpos;
			rpos += item.size = sztbl[j];
			if (fwrite(&item, sizeof item, 1, f) != 1) {
				perror(argv[i]);
				return 1;
			}
		}
	}
	return 0;
}

static int packtype(FILE *f, size_t rpos, unsigned nwave, unsigned type, char **argv, int argp, int argc, unsigned *itbl, size_t *sztbl)
{
	if (!nwave) return 0;
	char cpybuf[4096];
	long pos = ftell(f);
	size_t wpos = rpos;
	int ret = 1, fd = -1;
	if (pos < 0) {
		perror("can't pack");
		goto fail;
	}
	if ((size_t)pos != wpos) {
		fprintf(stderr, "mismatch: %zu, %zu\n", pos, wpos);
		goto fail;
	}
	for (int i = argp, j = 0; i < argc; ++i, ++j) {
		if (itbl[j] == type) {
			struct stat st;
			fd = open(argv[i], O_RDONLY);
			if (fd == -1) {
				perror(argv[i]);
				goto fail;
			}
			if (fstat(fd, &st) || (size_t)st.st_size != sztbl[j]) {
				perror("broken file system or MITM attack");
				goto fail;
			}
			for (size_t ii = 0; ii < sztbl[j];) {
				size_t n = sztbl[j] - ii;
				if (n > sizeof cpybuf)
					n = sizeof cpybuf;
				if ((size_t)read(fd, cpybuf, n) != n || fwrite(cpybuf, 1, n, f) != n) {
					perror("i/o error");
					goto fail;
				}
				ii += n;
			}
			close(fd);
		}
	}
	ret = 0;
fail:
	if (fd != -1)
		close(fd);
	return ret;
}

static int pack(int argp, int argc, char **argv)
{
	FILE *f = NULL;
	int ret = 1;
	struct drs_hdr hdr = {
#ifdef DRS_CUSTOM_HDR
		"Copyleft libregenie 2016",
#else
		"Copyright (c) 1997 Ensemble Studios\x2e\x1a",
#endif
		"1.00tribe", 0, 0
	};
	f = fopen(packname, "wb");
	if (!f) goto ioerr;
	struct stat st;
	unsigned nwave, nshp, nslp, nbin;
	uint32_t wavesz, shpsz, slpsz, binsz;
	unsigned nres = argc - argp + 1;
	unsigned *itbl = alloca(nres * sizeof(unsigned));
	unsigned *ntbl = alloca(nres * sizeof(unsigned));
	size_t *sztbl = alloca(nres * sizeof(size_t));
	nwave = nshp = nslp = nbin = 0;
	wavesz = shpsz = slpsz = binsz = 0;
	for (int i = argp, j = 0; i < argc; ++i, ++j) {
		if (stat(argv[i], &st)) {
			perror(argv[i]);
			goto fail;
		}
		const char *ext = strrchr(argv[i], '.');
		if (!ext) {
			++nbin;
			continue;
		}
		const char *num = ext - 1;
		while (num > argv[i] && isdigit(num[-1]))
			--num;
		ntbl[j] = 0; // default
		if (sscanf(num, "%u", &ntbl[j]) != 1) {
			fprintf(stderr, "missing id in: %s\n", argv[i]);
			goto fail;
		}
		++ext;
		if (!strcmp(ext, "wav")) {
			itbl[j] = DT_WAVE;
			++nwave;
			wavesz += st.st_size;
		} else if (!strcmp(ext, "shp")) {
			itbl[j] = DT_SHP;
			++nshp;
			shpsz += st.st_size;
		} else if (!strcmp(ext, "slp")) {
			itbl[j] = DT_SLP;
			++nslp;
			slpsz += st.st_size;
		} else {
			itbl[j] = DT_BINARY;
			++nbin;
			binsz += st.st_size;
		}
		sztbl[j] = st.st_size;
	}
	unsigned nlist = 0;
	if (nbin) ++nlist;
	if (nshp) ++nlist;
	if (nslp) ++nlist;
	if (nwave) ++nlist;
	printf("objects: %u (%u, 0x%X)\n", nbin, binsz, binsz);
	printf("shapes: %u (%u, 0x%X)\n", nshp, shpsz, shpsz);
	printf("models: %u (%u, 0x%X)\n", nslp, slpsz, slpsz);
	printf("audio: %u (%u, 0x%X)\n", nwave, wavesz, wavesz);
	printf("lists: %u\n", nlist);
	off_t pos = sizeof(struct drs_hdr);
	uint32_t listend = pos + nlist * sizeof(struct drs_list)
		+ (nwave + nshp + nslp + nbin) * sizeof(struct drs_item);
	hdr.nlist = nlist;
	hdr.listend = listend;
	if (fwrite(&hdr, sizeof hdr, 1, f) != 1)
		goto ioerr;
	// write drs lists
	pos += nlist * sizeof(struct drs_list);
	if (!packlist(f, DT_BINARY, &pos, nbin))
		goto ioerr;
	if (!packlist(f, DT_SHP, &pos, nshp))
		goto ioerr;
	if (!packlist(f, DT_SLP, &pos, nslp))
		goto ioerr;
	if (!packlist(f, DT_WAVE, &pos, nwave)) {
ioerr:
		perror(packname);
		goto fail;
	}
	long fpos = ftell(f);
	if (fpos < 0) {
		perror("can't pack");
		goto fail;
	}
	size_t ppos = fpos;
	// compute resource offset
	size_t rpos = ppos + (nwave + nshp + nslp + nbin) * sizeof(struct drs_item);
	size_t rposp = rpos;
	// write lookup tables
	if (packtbl(f, rpos, nbin, DT_BINARY, argv, argp, argc, itbl, ntbl, sztbl))
		goto ioerr;
	rpos += binsz;
	if (packtbl(f, rpos, nshp, DT_SHP, argv, argp, argc, itbl, ntbl, sztbl))
		goto ioerr;
	rpos += shpsz;
	if (packtbl(f, rpos, nslp, DT_SLP, argv, argp, argc, itbl, ntbl, sztbl))
		goto ioerr;
	rpos += slpsz;
	if (packtbl(f, rpos, nwave, DT_WAVE, argv, argp, argc, itbl, ntbl, sztbl))
		goto ioerr;
	rpos += wavesz;
	// write resources
	rpos = rposp;
	if (packtype(f, rpos, nbin, DT_BINARY, argv, argp, argc, itbl, sztbl))
		goto fail;
	rpos += binsz;
	if (packtype(f, rpos, nshp, DT_SHP, argv, argp, argc, itbl, sztbl))
		goto fail;
	rpos += shpsz;
	if (packtype(f, rpos, nslp, DT_SLP, argv, argp, argc, itbl, sztbl))
		goto fail;
	rpos += slpsz;
	if (packtype(f, rpos, nwave, DT_WAVE, argv, argp, argc, itbl, sztbl))
		goto fail;
	ret = 0;
fail:
	if (f) fclose(f);
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
	if (argp < 0)
		return 1;
	if (opts & OPT_PACK)
		return pack(argp, argc, argv);
	for (; argp < argc; ++argp) {
		puts(argv[argp]);
		if (process(argv[argp]))
			return 1;
	}
	return 0;
}
