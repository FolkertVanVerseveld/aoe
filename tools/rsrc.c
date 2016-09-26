#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "xmap.h"

static int process(char *name)
{
	int ret, fd = -1;
	char *map = MAP_FAILED;
	size_t mapsz = 0;
	struct xfile x;
	if (xmap(name, PROT_READ, &fd, &map, &mapsz))
		return 1;
	if ((ret = xstat(&x, map, mapsz)) != 0) {
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
