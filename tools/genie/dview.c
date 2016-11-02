#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include "../../genie/drs.h"

static struct option long_opt[] = {
	{"help", no_argument, 0, 0},
	{"version", no_argument, 0, 0},
	{0, 0, 0, 0},
};

#define usage(f,n) fprintf(f, "usage: %s file\n", n)

static int parse_opt(int argc, char **argv)
{
	int c;
	while (1) {
		int option_index;
		c = getopt_long(argc, argv, "ht", long_opt, &option_index);
		if (c == -1) break;
		switch (c) {
		case 'h':
			usage(stdout, argc > 0 ? argv[0] : "view");
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
	ret = 0;
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
	for (; argp < argc; ++argp)
		if (process(argv[argp]))
			return 1;
	return 0;
}
