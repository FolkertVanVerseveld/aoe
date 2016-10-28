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

#define DRS_LISTSZ 8

static struct dmap drs_list[DRS_LISTSZ];
static unsigned drs_count = 0;

void dmap_init(struct dmap *map) {
	map->dblk = NULL;
	map->drs_data = NULL;
	map->filename[0] = '\0';
}

void dmap_free(struct dmap *map) {
	assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
	if (map->drs_data) free(map->drs_data);
	if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
}

void drs_free(void) {
	for (unsigned i = 0; i < drs_count; ++i)
		dmap_free(&drs_list[i]);
	drs_count = 0;
}

static inline void str2(char *buf, size_t n, const char *s1, const char *s2)
{
	snprintf(buf, n, "%s%s", s1, s2);
}

int read_data_mapping(const char *filename, const char *directory, int nommap)
{
	struct dbuf {
		char data[60];
		uint32_t length;
	} buf;
	struct dmap *map = NULL;
	char name[DMAPBUFSZ];
	int fd = -1;
	int ret = -1;
	ssize_t n;
	if (drs_count == DRS_LISTSZ) {
		fputs("overflow: drs_list\n", stderr);
		goto fail;
	}
	map = &drs_list[drs_count];
	str2(name, sizeof name, directory, filename);
	// deze twee regels komt dubbel voor in dissassembly
	// aangezien ik niet van duplicated code hou staat het nu hier
	fd = open(name, O_RDONLY); // XXX verify mode
	if (fd == -1) goto fail;
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
	// check for magic "1.00tribe"
	ret = strncmp(map->drs_data->version, "1.00tribe", strlen("1.00tribe"));
	if (ret) fprintf(stderr, "%s: hdr invalid\n", filename);
	// insert map into drs list
	++drs_count;
fail:
	if (fd != -1) close(fd);
	if (ret && map) {
		assert((map->drs_data && !map->dblk) || map->drs_data == map->dblk);
		if (map->drs_data) free(map->drs_data);
		if (map->dblk && map->dblk != map->drs_data) free(map->dblk);
		perror(name);
	}
	return ret;
}

static int drs_map(unsigned type, int res_id, int *fd, off_t *offset, char **dblk, size_t *count)
{
	struct drsmap *drs_data;
	struct dmap *map = drs_list;
	unsigned drs_i = 0;
	if (drs_count) {
		for (drs_i = 0; drs_i < drs_count; ++drs_i) {
			drs_data = map->drs_data;
			map = &drs_list[drs_i];
			if (!drs_data->nlist)
				continue;
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
						*dblk = (char*)map->dblk;
						*count = item->size;
						return 1;
					}
				}
			}
		}
	}
fail:
	dbgf("%s: not found: type=%X, res_id=%d\n", __func__, type, res_id);
	return 0;
}

void *drs_get_item(unsigned type, int res_id, size_t *count, off_t *offset)
{
	char *data;
	int dummy;
	if (!drs_map(type, res_id, &dummy, offset, &data, count))
		return NULL;
	if (!data) {
		fprintf(stderr, "no data for %u\n", res_id);
		halt();
	}
	return data + *offset;
}
