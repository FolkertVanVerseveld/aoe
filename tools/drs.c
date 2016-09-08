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
#define DT_SHP    0x73687020
#define DT_SLP    0x736c7020
#define DT_BINARY 0x62696e61

struct drs {
	uint32_t v0, v1;
	uint32_t v2;
	uint32_t type;
	uint32_t v4;
};

struct drs_list {
	uint32_t size;
	uint32_t type;
	uint32_t offset;
};

struct drs_item {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct drsmap {
	char copyright[40];
	char version[12];
	uint32_t num34;
	uint32_t count38;
	//struct drs_res res[]
};

static inline void dump_type(unsigned type)
{
	switch (type) {
		case DT_WAVE:
			puts("type = wave");
			break;
		case DT_SHP:
			puts("type = shp");
			break;
		case DT_SLP:
			puts("type = slp");
			break;
		case DT_BINARY:
			puts("type = binary");
			break;
		default:
			printf("type = %x\n", type);
			break;
	}
}

void drs_dump(const struct drs *this)
{
	printf(
		"v0 = %x\n"
		"v1 = %x\n"
		"v2 = %x\n",
		this->v0, this->v1, this->v2
	);
	dump_type(this->type);
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

#define WAVE_CHUNK_ID 0x46464952

void drs_find_wave(char *data, size_t size)
{
	size_t off;
	uint32_t magic = WAVE_CHUNK_ID;
	FILE *f = NULL;
	uint32_t f_size;
	char buf[256];
	unsigned waves = 0;
	for (off = 0; off < size - sizeof magic; ++off) {
		if (*(uint32_t*)&data[off] == magic) {
			f_size = *(uint32_t*)&data[off + sizeof(uint32_t)] + 8;
			snprintf(buf, sizeof buf, "%zX.wav", off);
			f = fopen(buf, "wb");
			if (!f) return;
			fwrite(&data[off], 1, f_size, f);
			fclose(f);
			f = NULL;
			++waves;
		}
	}
	printf("exported wave count: %u\n", waves);
	if (f) fclose(f);
}

void drsmap_stat(char *data, size_t size)
{
	if (size < sizeof(struct drsmap)) {
		fputs("invalid drs file\n", stderr);
		return;
	}
	struct drsmap *drs = (struct drsmap*)data;
	if (strncmp("1.00tribe", drs->version, strlen("1.00tribe"))) {
		fputs("invalid drs file\n", stderr);
		return;
	}
	printf("num34: %u\n", drs->num34);
	printf("count38: %u\n", drs->count38);
	if (!drs->count38)
		return;
	struct drs_list *list = (struct drs_list*)((char*)drs + sizeof(struct drsmap));
	for (unsigned i = 0; i < drs->count38; ++i, ++list) {
		printf("size: %x\n", list->size);
		dump_type(list->type);
		printf("offset: %x\n", list->offset);
		// resource offset or something
		if (list->offset > size) {
			fputs("bad item offset\n", stderr);
			return;
		}
		struct drs_item *item = (struct drs_item*)(data + list->offset);
		for (unsigned j = 0; j < list->size; ++j, ++item) {
			if (list->offset + (j + 1) * sizeof(struct drs_item) > size) {
				fputs("bad list\n", stderr);
				return;
			}
			printf("#%u\n", j);
			printf("item ID: %u\n", item->id);
			printf("offset: %u\n", item->offset);
			printf("size: %u\n", item->size);
		}
	}
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
	drsmap_stat(map, mapsz);
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
