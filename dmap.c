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
#include "dbg.h"
#include "todo.h"

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

static int drs_map(const char *dest, int a2, int *fd, off_t *st_size, const char **dblk, size_t *count)
{
	stub
	#if 1
	(void)dest;
	(void)a2;
	(void)fd;
	(void)st_size;
	(void)dblk;
	(void)count;
	halt();
	return 0;
	#else
	char *v8, *v9;
	int v10, result;
	struct dmap *map = drs_list, *map2;
	*fd = -1;
	*st_size = 0;
	*dblk = NULL;
	map2 = map;
	int v7, a2a;
	*count = 0;
	if (map) {
		v7 = a2;
		while (1) {
			v8 = map->drs_data;
			a2a = 0;
			if (*((int*)v8 + 14) > 0)
				break;
next:
			map = map->next;
			map2 = map;
			if (!map)
				goto fail;
		}
		// FIXME
		v9 = v8 + 72;
		while (1) {
			if (v9[-2] == dest) {
				v10 = 0;
				if (*v9 > 0)
					break;
			}
LABEL_11:
			v9 += 12;
			if (++a2a >= *((int*)v8 + 14))
				goto next;
		}
		v11 = &v8[*((int*)v9 - 1)];
		while ( *v11 != v7 )
		{
			++v10;
			v11 += 12;
			if ( v10 >= *v9 )
			{
				map = map2;
				goto LABEL_11;
			}
		}
		char *v13 = &v8[12 * v10] + *(v9 - 1);
		*fd = map2->fd;
		*st_size = *(v13 + 1);
		*dblk = map2->dblk;
		*count = *(v13 + 2);
		result = 1;
	} else
		goto fail;
	return 1;
fail:
	fprintf(stderr, "drs_map failed: a2=%d\n", a2);
	return 0;
	#endif
}

void *drs_get_item(const char *item, int fd, size_t *count, off_t *offset)
{
	stub
	int v7, result;
	#if 1
	(void)item;
	(void)fd;
	(void)count;
	(void)offset;
	(void)result;
	v7 = fd;
	drs_map(item, v7, &fd, offset, &item, count);
	halt();
	return NULL;
	#else
	size_t *local_count;
	const char *v9;
	off_t *v5, v10;
	size_t *v11, v13, *v14;
	void *v12 = NULL;

	local_count = count;                          // XXX can we remap this to count?
	v5 = offset;
	v7 = fd;
	*count = -1;
	*v5 = 0;
	result = drs_map(item, v7, &fd, &offset, &item, &count);
	if (result) {
		v9 = item;
		if (item) {
			v10 = offset;
			v11 = count;
			*local_count = 0;
			*v5 = v11;
			return &v9[v10];
		}
		v13 = *count;
		*local_count = 1;
		v12 = malloc(v13);
		assert(v12);
		*v5 = count;
		if (result) {
			lseek(fd, *offset, SEEK_SET);
			v14 = read(fd, v12, count);
			if (v14 == count)
				return v12;
			result = 0;
		}
	}
	return v12;
	#endif
}
