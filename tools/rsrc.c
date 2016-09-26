#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "xmap.h"

static int rsrc_walk(struct xfile *x, unsigned level, size_t soff, size_t *off)
{
	char *map = x->data;
	size_t size = x->size;
	if (*off + sizeof(struct rsrcdir) > size) {
		fprintf(stderr, "bad rsrc node at level %u: file too small\n", level);
		return 1;
	}
	if (level == 3) {
		fputs("bad rsrc: too many levels\n", stderr);
		return 1;
	}
	struct rsrcdir *d = (struct rsrcdir*)(map + *off);
	printf("%8zX ", *off);
	for (unsigned l = 0; l < level; ++l)
		fputs("  ", stdout);
	printf(
		"ResDir Named:%02X ID:%02X TimeDate:%8X Vers:%u.%02u Char:%u\n",
		d->r_nnment, d->r_nident, d->r_timdat, d->r_major, d->r_minor, d->r_flags
	);
	unsigned n = d->r_nnment + d->r_nident;
	*off += sizeof(struct rsrcdir);
	if (*off + n * sizeof(struct rsrcdir) > size) {
		fprintf(stderr, "bad rsrc node at level %u: file too small\n", level);
		return 1;
	}
	int dir = 0;
	for (unsigned i = 0; i < n; ++i) {
		struct rsrcditem *ri = (struct rsrcditem*)(map + *off);
		if (dir) {
			if (rsrc_walk(x, level + 1, soff, off))
				return 1;
		} else if ((ri->r_rva >> 31) & 1) {
			size_t roff = soff + (ri->r_rva & ~(1 << 31));
			if (rsrc_walk(x, level + 1, soff, &roff))
				return 1;
			*off = roff;
			dir = 1;
		} else {
			printf("%8zX ", *off);
			for (unsigned l = 0; l < level; ++l)
				fputs("  ", stdout);
			printf("#%u ID: %8X Offset: %8X\n", i, ri->r_id, ri->r_rva);
			*off += sizeof(struct rsrcditem);
			return 0;
		}
	}
	return 0;
}

static int rsrc_stat(struct xfile *x)
{
	struct sechdr *sec = x->sec;
	if (!sec) {
		fputs("internal error\n", stderr);
		return 1;
	}
	unsigned i;
	for (i = 0; i < x->peopt->o_nrvasz; ++i, ++sec) {
		char name[9];
		strncpy(name, sec->s_name, 9);
		name[8] = '\0';
		if (!strcmp(name, ".rsrc"))
			goto found;
	}
	fputs("no .rsrc\n", stderr);
	return 1;
found:
	printf("goto %u@%zX\n", i, (size_t)sec->s_scnptr);
	size_t off = sec->s_scnptr;
	return rsrc_walk(x, 0, sec->s_scnptr, &off);
}

static int process(char *name)
{
	int ret = 1, fd = -1;
	char *map = MAP_FAILED;
	size_t mapsz = 0;
	struct xfile x;
	if (xmap(name, PROT_READ, &fd, &map, &mapsz))
		return 1;
	if ((ret = xstat(&x, map, mapsz)) != 0)
		goto fail;
	if (x.type != XT_PEOPT) {
		fprintf(stderr, "%s: type %u, expected %u\n", name, x.type, XT_PEOPT);
		goto fail;
	}
	ret = rsrc_stat(&x);
fail:
	if (ret) {
		xunmap(fd, map, mapsz);
		return ret;
	}
	return xunmap(fd, map, mapsz);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fputs("usage: rsrc file\n", stderr);
		return 1;
	}
	for (int i = 1; i < argc; ++i) {
		puts(argv[i]);
		if (process(argv[i]))
			return 1;
	}
	return 0;
}
