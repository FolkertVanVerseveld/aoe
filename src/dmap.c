#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "dmap.h"
#include "dbg.h"
#include "todo.h"
#include "../genie/drs.h"

#define DRS_LISTSZ 8

static struct dmap drs_list[DRS_LISTSZ];
static unsigned drs_count = 0;

static inline void dmap_init(struct dmap *this)
{
	this->fd = -1;
	this->data = NULL;
	this->length = 0;
	this->filename[0] = '\0';
	this->nommap = 0;
}

static inline void dmap_free(struct dmap *map)
{
	if (map->nommap) {
		if (map->data) {
			free(map->data);
			map->data = NULL;
		}
	} else if (map->data) {
		munmap(map->data, map->length);
		map->data = NULL;
	}
	if (map->fd != -1) {
		close(map->fd);
		map->fd = -1;
	}
}

void drs_free(void)
{
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
	struct dmap *map = NULL;
	char name[DMAPBUFSZ];
	int fd = -1, ret = 1;
	ssize_t n;
	if (drs_count == DRS_LISTSZ) {
		fputs("overflow: drs_list\n", stderr);
		goto fail;
	}
	map = &drs_list[drs_count];
	dmap_init(map);
	str2(name, sizeof name, directory, filename);
	// deze twee regels komt dubbel voor in dissassembly
	// aangezien ik niet van duplicated code hou staat het nu hier
	fd = open(name, O_RDONLY); // XXX verify mode
	struct stat st;
	if (fd == -1 || fstat(fd, &st))
		goto fail;
	map->length = st.st_size;
	if (nommap) {
		map->nommap = 1;
		map->data = malloc(st.st_size);
		if (!map->data) {
			perror(__func__);
			goto fail;
		}
		n = read(fd, map->data, st.st_size);
		if (n != st.st_size) {
			perror(__func__);
			goto fail;
		}
	} else {
		char *data = mmap(NULL, map->length, PROT_READ, MAP_PRIVATE, fd, 0);
		if (data == MAP_FAILED) {
			perror(filename);
			goto fail;
		}
		map->data = data;
		/* transfer file ownership to map */
		map->fd = fd;
		fd = -1;
	}
	if (strlen(filename) >= DMAPBUFSZ) {
		fprintf(stderr, "map: overflow: %s\n", name);
		goto fail;
	}
	strcpy(map->filename, filename);
	// check for magic "1.00tribe"
	ret = strncmp(((struct drsmap*)map->data)->version, "1.00tribe", strlen("1.00tribe"));
	if (ret) fprintf(stderr, "%s: hdr invalid\n", filename);
	// insert map into drs list
	++drs_count;
fail:
	if (ret) {
		dmap_free(map);
		perror(name);
	}
	if (fd != -1)
		close(fd);
	return ret;
}

static int drs_map(unsigned type, int res_id, off_t *offset, char **dblk, size_t *count)
{
	if (!drs_count)
		goto fail;
	for (unsigned drs_i = 0; drs_i < drs_count; ++drs_i) {
		struct dmap *map = &drs_list[drs_i];
		struct drsmap *drs = (struct drsmap*)map->data;
		if (!drs->nlist)
			continue;
		struct drs_list *list = (struct drs_list*)((char*)drs + sizeof(struct drsmap));
		for (unsigned i = 0; i < drs->nlist; ++i, ++list) {
			if (list->type != type)
				continue;
			if (list->offset > map->length) {
				fputs("bad item offset\n", stderr);
				goto fail;
			}
			struct drs_item *item = (struct drs_item*)((char*)drs + list->offset);
			for (unsigned j = 0; j < list->size; ++j, ++item) {
				if (list->offset + (j + 1) * sizeof(struct drs_item) > map->length) {
					fputs("bad list\n", stderr);
					goto fail;
				}
				if (item->id == (unsigned)res_id) {
					dbgf("drs map: %u from %s\n", res_id, map->filename);
					*offset = item->offset;
					*dblk = map->data;
					*count = item->size;
					return 1;
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
	if (!drs_map(type, res_id, offset, &data, count))
		return NULL;
	if (!data) {
		fprintf(stderr, "no data for %u\n", res_id);
		halt();
	}
	return data + *offset;
}
