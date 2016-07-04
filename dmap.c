#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "dmap.h"

struct dmap *drs_list = NULL;
static const char data_map_magic[4] = {'1', '.', '0', '0'};

void dmap_init(struct dmap *map) {
	map->dblk = map->drs_data = NULL;
	map->filename[0] = '\0';
	map->next = NULL;
}

void dmap_free(struct dmap *map) {
	assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
	if (map->drs_data) free(map->drs_data);
	if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
	if (map) free(map);
}

void drs_free(void) {
	if (!drs_list) return;
	struct dmap *next, *item = drs_list;
	do {
		next = item->next;
		if (item) dmap_free(item);
		item = next;
	} while (item);
}

static inline void str2(char *buf, size_t n, const char *s1, const char *s2)
{
	snprintf(buf, n, "%s%s", s1, s2);
}

int read_data_mapping(const char *filename, const char *directory, int nommap)
{
	// data types that are read from drs files must honor WIN32 ABI
	typedef uint32_t size32_t;
	struct dbuf {
		char data[60];
		size32_t length;
	} buf;
	struct dmap *map = NULL;
	char name[DMAPBUFSZ];
	int fd = -1;
	int ret = -1;
	ssize_t n;
	str2(name, sizeof name, directory, filename);
	// deze twee regels komt dubbel voor in dissassembly
	// aangezien ik niet van duplicated code hou staat het nu hier
	fd = open(name, O_RDONLY); // XXX verify mode
	if (fd == -1) goto fail;
	map = malloc(sizeof *map);
	if (!map) {
		perror(__func__);
		goto fail;
	}
	dmap_init(map);
	if (!nommap) {
		struct stat st;
		if (fstat(fd, &st)) goto fail;
		map->length = st.st_size;
		map->dblk = malloc(st.st_size);
		if (!map->dblk) {
			perror(__func__);
			goto fail;
		}
		n = read(fd, map->dblk, st.st_size);
		if (n != st.st_size) {
			perror(__func__);
			ret = n;
			goto fail;
		}
	}
	if (strlen(filename) >= DMAPBUFSZ) {
		fprintf(stderr, "map: overflow: %s\n", name);
		goto fail;
	}
	strcpy(map->filename, filename);
	puts(name);
	if (nommap) {
		map->fd = fd;
		map->dblk = NULL;
		lseek(fd, 0, SEEK_CUR);
		n = read(fd, &buf, sizeof buf);
		if (n != sizeof buf) {
			fprintf(stderr, "%s: bad map\n", name);
			ret = n;
			goto fail;
		}
		map->length = buf.length;
		map->drs_data = malloc(buf.length);
		if (!map->drs_data) {
			perror(__func__);
			goto fail;
		}
		lseek(fd, 0, SEEK_SET);
		n = read(fd, map->drs_data, buf.length);
		if (n != buf.length) {
			fprintf(stderr, "%s: bad map\n", name);
			ret = n;
			goto fail;
		}
	} else {
		map->fd = -1;
		map->drs_data = map->dblk;
	}
	assert(map->drs_data);
	// insert map into drs list
	if (drs_list) {
		struct dmap *last = drs_list;
		while (last->next) last = last->next;
		last->next = map;
	} else {
		drs_list = map;
	}
	// check for magic "1.00tribe"
	ret = strncmp(&map->drs_data[0x28], "1.00tribe", strlen("1.00tribe"));
	if (ret) fprintf(stderr, "%s: hdr invalid\n", filename);
	// FIXME if no magic, dangling ptr in last item of drs list
fail:
	if (fd != -1) close(fd);
	if (ret && map) {
		assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
		if (map->drs_data) free(map->drs_data);
		if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
		if (map) free(map);
		perror(name);
	}
	return ret;
}
