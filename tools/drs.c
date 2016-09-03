#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define DT_WAVE   0x77617620
#define DT_SLP    0x736c7020
#define DT_BINARY 0x62696e61

struct drs {
	uint32_t v0, v1;
	uint32_t v2;
	uint32_t type;
	uint32_t v4;
};

void drs_dump(const struct drs *this)
{
	printf(
		"v0 = %x\n"
		"v1 = %x\n"
		"v2 = %x\n",
		this->v0, this->v1, this->v2
	);
	switch (this->type) {
		case DT_WAVE:
			puts("type = wave");
			break;
		case DT_SLP:
			puts("type = slp");
			break;
		case DT_BINARY:
			puts("type = binary");
			break;
		default:
			printf("type = %x\n", this->type);
			break;
	}
	printf(
		"v4 = %x\n",
		this->v4
	);
}

void drs_stat(char *data, size_t size)
{
	size_t n = strlen("1.00tribe");
	if (n > size || strncmp("1.00tribe", &data[0x28], n)) {
		fputs("invalid drs file\n", stderr);
		return;
	}
	struct drs *drs = (void*)&data[0x34];
	drs_dump(drs);
}

static int process(char *name)
{
	int ret = 1, fd = -1;
	size_t mapsz;
	char *map = MAP_FAILED;
	fd = open(name, O_RDONLY);
	struct stat st;
	if (fd == -1 || fstat(fd, &st) == -1) {
		perror(name);
		goto fail;
	}
	map = mmap(NULL, mapsz = st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		perror("mmap");
		goto fail;
	}
	drs_stat(map, mapsz);
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
	if (argc < 2) {
		fputs("usage: drs file\n", stderr);
		return 1;
	}
	for (int i = 1; i < argc; ++i) {
		puts(argv[i]);
		if (process(argv[i]))
			return 1;
	}
	return 0;
}
