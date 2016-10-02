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
	map->dblk = NULL;
	map->drs_data = NULL;
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
	ret = strncmp(map->drs_data->version, "1.00tribe", strlen("1.00tribe"));
	if (ret) fprintf(stderr, "%s: hdr invalid\n", filename);
	// FIXME if no magic, dangling ptr in last item of drs list
fail:
	if (fd != -1) close(fd);
	if (ret && map) {
		assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
		if (map->drs_data) free(map->drs_data);
		if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
		free(map);
		perror(name);
	}
	return ret;
}

static int drs_map(unsigned type, int res_id, int *fd, off_t *offset, unsigned *dblk, size_t *count)
{
	struct drsmap *drs_data;
	struct dmap *map = drs_list;
	if (map) {
		while (1) {
			drs_data = map->drs_data;
			if (!map->next)
				goto fail;
			if (!drs_data->nlist)
				goto next;
			struct drs_list *list = (struct drs_list*)((char*)drs_data + sizeof(struct drsmap));
			for (unsigned i = 0; i < drs_data->nlist; ++i, ++list) {
				if (list->type != type)
					continue;
				if (list->offset > map->length) {
					fputs("bad item offset\n", stderr);
					goto fail;
				}
				struct drs_item *item = (struct drs_item*)((char*)drs_data + list->offset);
				for (unsigned j = 0; j < list->size; ++j, ++item) {
					if (list->offset + (j + 1) * sizeof(struct drs_item) > map->length) {
						fputs("bad list\n", stderr);
						goto fail;
					}
					if (item->id == (unsigned)res_id) {
						dbgf("drs map: %u from %s\n", res_id, map->filename);
						*fd = map->fd;
						*offset = item->offset;
						// XXX V bogus
						//*dblk = map->dblk;
						*count = item->size;
						return 1;
					}
				}
			}
		next:
			map = map->next;
		}
	}
fail:
	dbgf("%s: not found: type=%X, res_id=%d\n", __func__, type, res_id);
	return 0;
}

void *drs_get_item(unsigned item, int fd, size_t *count, off_t *offset)
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
