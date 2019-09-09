/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "drs.h"

#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/fs.h>
#include <genie/memory.h>

#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#ifndef _WIN32
	#include <sys/mman.h>
#else
	#include <windows.h>
	#include <io.h>
#endif
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
	#define INVALID_MAPPING NULL
#else
	#define INVALID_MAPPING MAP_FAILED
#endif

#define DRS_MAX 16

struct drs_map {
	int fd;
	void *data;
	struct stat st;
	char *name;
};

struct drs_files {
	struct drs_map *data;
	size_t cap, count;
} drs_files;

void drs_map_init(struct drs_map *drs, const char *name);
void drs_map_open(struct drs_map *drs);
void drs_map_free(struct drs_map *drs);

void drs_files_init(struct drs_files *lst, size_t cap)
{
	lst->data = mem_alloc(cap * sizeof(struct drs_map));
	lst->cap = cap;
	lst->count = 0;
}

void drs_files_free(struct drs_files *lst)
{
	for (size_t i = 0, n = lst->count; i < n; ++i)
		drs_map_free(&lst->data[i]);
	mem_free(lst->data);
}

void drs_files_add(struct drs_files *lst, const char *name)
{
	if (lst->count == lst->cap) {
		size_t n = 2 * lst->cap;
		if (!n)
			n = 8;

		lst->data = mem_realloc(lst->data, n * sizeof(struct drs_map));
		lst->cap = n;
	}
	drs_map_init(&lst->data[lst->count++], name);
}

void drs_files_open(struct drs_files *lst)
{
	for (size_t i = 0, n = lst->count; i < n; ++i)
		drs_map_open(&lst->data[i]);
}

void drs_map_init(struct drs_map *drs, const char *name)
{
	char buf[FS_BUFSZ];
	fs_data_path(buf, sizeof buf, name);
	drs->name = strdup(buf);
}

void drs_map_open(struct drs_map *drs)
{
	if ((drs->fd = open(drs->name, O_RDONLY)) == -1 || fstat(drs->fd, &drs->st) == -1)
		panicf("%s: %s\n", drs->name, strerror(errno));

#ifndef _WIN32
	drs->data = mmap(NULL, drs->st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, drs->fd, 0);
	if (drs->data == INVALID_MAPPING)
		panicf("%s: cannot map: %s\n", drs->name, strerror(errno));
#else
	HANDLE fm, h;
	DWORD protect = PAGE_READONLY, desiredAccess = FILE_MAP_READ;
	DWORD dwMaxSizeLow, dwMaxSizeHigh;

	dwMaxSizeLow = sizeof(off_t) <= sizeof(DWORD) ? (DWORD)drs->st.st_size : (DWORD)(drs->st.st_size & 0xFFFFFFFFL);
	dwMaxSizeHigh = sizeof(off_t) <= sizeof(DWORD) ? (DWORD)0 : (DWORD)((drs->st.st_size >> 32) & 0xFFFFFFFFL);

	if ((h = (HANDLE)_get_osfhandle(drs->fd)) == INVALID_MAPPING || !(fm = CreateFileMapping(h, NULL, protect, dwMaxSizeHigh, dwMaxSizeLow, NULL)))
		panicf("%s: cannot map file\n", drs->name);

	if (!(drs->data = MapViewOfFile(fm, desiredAccess, 0, 0, drs->st.st_size)))
		panicf("%s: cannot map: unknown error\n", drs->name);

	// store the handles somewhere
	//CloseHandle(fm);
	//CloseHandle(h);
#endif

	// verify data
	const struct drs_hdr *hdr = drs->data;
	if (strncmp(hdr->version, "1.00tribe", strlen("1.00tribe")))
		panicf("%s: not a DRS file\n", drs->name);

}

void drs_map_free(struct drs_map *drs)
{
	free(drs->name);

	if (drs->data != INVALID_MAPPING)
#ifndef _WIN32
		munmap(drs->data, drs->st.st_size);
#else
		if (!UnmapViewOfFile(drs->data))
			fprintf(stderr, "Cannot unmap address %p\n", drs->data);
#endif

	if (drs->fd != -1)
		close(drs->fd);
}

void drs_add(const char *name)
{
	drs_files_add(&drs_files, name);
}

void drs_init(void)
{
	drs_files_open(&drs_files);
}

void drs_free(void)
{
	drs_files_free(&drs_files);
}

#define drs_map_size(map) ((size_t)map->st.st_size)

int drs_files_map(const struct drs_files *lst, unsigned type, unsigned res_id, off_t *offset, char **dblk, size_t *count)
{
	for (size_t i = 0, n = lst->count; i < n; ++i) {
		struct drs_map *map = &lst->data[i];
		struct drs_hdr *hdr = map->data;

		struct drs_list *list = (struct drs_list*)((char*)hdr + sizeof(struct drs_hdr));
		for (size_t j = 0; j < hdr->nlist; ++j, ++list) {
			if (list->type != type)
				continue;
			if (list->offset > drs_map_size(map)) {
				fputs("bad item offset\n", stderr);
				goto fail;
			}
			struct drs_item *item = (struct drs_item*)((char*)hdr + list->offset);
			for (unsigned k = 0; k < list->size; ++k, ++item) {
				if (list->offset + (k + 1) * sizeof(struct drs_item) > drs_map_size(map)) {
					fputs("bad list\n", stderr);
					goto fail;
				}
				if (item->id == res_id) {
					dbgf("drs map: %u from %s\n", res_id, map->name);
					*offset = item->offset;
					*dblk = map->data;
					*count = item->size;
					return 0;
				}
			}
		}
	}
fail:
	dbgf("%s: not found: type=%X, res_id=%d\n", __func__, type, res_id);
	return 1;
}

const void *drs_get_item(unsigned type, unsigned id, size_t *count)
{
	char *data;
	off_t offset;

	if (drs_files_map(&drs_files, type, id, &offset, &data, count) || !data)
		panicf("Cannot find resource %d\n", id);

	return data + offset;
}

void slp_map(struct slp *dst, const void *data)
{
	dst->hdr = (void*)data;
	dst->info = (struct slp_frame_info*)((char*)data + sizeof(struct slp_header));
}

static unsigned cmd_or_next(const unsigned char **cmd, unsigned n)
{
	const unsigned char *ptr = *cmd;
	unsigned v = *ptr >> n;
	if (!v)
		v = *(++ptr);
	*cmd = ptr;
	return v;
}

bool slp_read(SDL_Surface *surface, const struct SDL_Palette *pal, const void *data, const struct slp_frame_info *frame, unsigned player)
{
	bool dynamic = false;

#if DEBUG_SLP
	dbgf("player: %u\n", player);

	dbgf("dimensions: %u x %u\n", frame->width, frame->height);
	dbgf("hotspot: %u,%u\n", frame->hotspot_x, frame->hotspot_y);
	dbgf("command table offset: %X\n", frame->cmd_table_offset);
	dbgf("outline table offset: %X\n", frame->outline_table_offset);
#endif

	if (!surface || SDL_SetSurfacePalette(surface, pal))
		panicf("Cannot create image: %s\n", SDL_GetError());
	if (surface->format->format != SDL_PIXELFORMAT_INDEX8)
		panicf("Unexpected image format: %s\n", SDL_GetPixelFormatName(surface->format->format));

	// fill with random data so we can easily spot garbled image data
	unsigned char *pixels = (unsigned char*)surface->pixels;
	for (int y = 0, h = frame->height, p = surface->pitch; y < h; ++y)
		for (int x = 0, w = frame->width; x < w; ++x)
			pixels[y * p + x] = 0;

	const struct slp_frame_row_edge *edge =
		(const struct slp_frame_row_edge*)
			((char*)data + frame->outline_table_offset);
	const unsigned char *cmd = (const unsigned char*)
		((char*)data + frame->cmd_table_offset + 4 * frame->height);

#if DEBUG_SLP
	dbgf("command data offset: %X\n", frame->cmd_table_offset + 4 * frame->height);
#endif

	for (int y = 0, h = frame->height; y < h; ++y, ++edge) {
		if (edge->left_space == 0x8000 || edge->right_space == 0x8000)
			continue;

		int line_size = frame->width - edge->left_space - edge->right_space;
#if DEBUG_SLP
		dbgf("%8X: %3d: %d (%hu, %hu)\n",
			(unsigned)(cmd - (const unsigned char*)data),
			y, line_size, edge->left_space, edge->right_space
		);
#endif

		// fill line_size with default value
		for (int x = edge->left_space, w = x + line_size, p = surface->pitch; x < w; ++x)
			pixels[y * p + x] = rand();

		for (int i = edge->left_space, x = i, w = x + line_size, p = surface->pitch; i <= w; ++i, ++cmd) {
			unsigned command, count;

			command = *cmd & 0x0f;

			// TODO figure out if lesser skip behaves different compared to aoe2

			switch (*cmd) {
			case 0x03:
				for (count = *++cmd; count; --count)
					pixels[y * p + x++] = 0;
				continue;
			case 0xF7: pixels[y * p + x++] = cmd[1];
			case 0xE7: pixels[y * p + x++] = cmd[1];
			case 0xD7: pixels[y * p + x++] = cmd[1];
			case 0xC7: pixels[y * p + x++] = cmd[1];
			case 0xB7: pixels[y * p + x++] = cmd[1];
			// dup 10
			case 0xA7: pixels[y * p + x++] = cmd[1];
			case 0x97: pixels[y * p + x++] = cmd[1];
			case 0x87: pixels[y * p + x++] = cmd[1];
			case 0x77: pixels[y * p + x++] = cmd[1];
			case 0x67: pixels[y * p + x++] = cmd[1];
			case 0x57: pixels[y * p + x++] = cmd[1];
			case 0x47: pixels[y * p + x++] = cmd[1];
			case 0x37: pixels[y * p + x++] = cmd[1];
			case 0x27: pixels[y * p + x++] = cmd[1];
			case 0x17: pixels[y * p + x++] = cmd[1];
				++cmd;
				continue;
			// player color fill
			case 0xF6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 15
			case 0xE6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 14
			case 0xD6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 13
			case 0xC6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 12
			case 0xB6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 11
			case 0xA6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 10
			case 0x96: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 9
			case 0x86: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 8
			case 0x76: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 7
			case 0x66: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 6
			case 0x56: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 5
			case 0x46: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 4
			case 0x36: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 3
			case 0x26: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 2
			case 0x16: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 1
				dynamic = true;
				continue;
			// XXX pixel count if lower_nibble == 4: 1 + cmd >> 2
			case 0xfc: pixels[y * p + x++] = *++cmd; // fill 63
			case 0xf8: pixels[y * p + x++] = *++cmd; // fill 62
			case 0xf4: pixels[y * p + x++] = *++cmd; // fill 61
			case 0xf0: pixels[y * p + x++] = *++cmd; // fill 60
			case 0xec: pixels[y * p + x++] = *++cmd; // fill 59
			case 0xe8: pixels[y * p + x++] = *++cmd; // fill 58
			case 0xe4: pixels[y * p + x++] = *++cmd; // fill 57
			case 0xe0: pixels[y * p + x++] = *++cmd; // fill 56
			case 0xdc: pixels[y * p + x++] = *++cmd; // fill 55
			case 0xd8: pixels[y * p + x++] = *++cmd; // fill 54
			case 0xd4: pixels[y * p + x++] = *++cmd; // fill 53
			case 0xd0: pixels[y * p + x++] = *++cmd; // fill 52
			case 0xcc: pixels[y * p + x++] = *++cmd; // fill 51
			case 0xc8: pixels[y * p + x++] = *++cmd; // fill 50
			case 0xc4: pixels[y * p + x++] = *++cmd; // fill 49
			case 0xc0: pixels[y * p + x++] = *++cmd; // fill 48
			case 0xbc: pixels[y * p + x++] = *++cmd; // fill 47
			case 0xb8: pixels[y * p + x++] = *++cmd; // fill 46
			case 0xb4: pixels[y * p + x++] = *++cmd; // fill 45
			case 0xb0: pixels[y * p + x++] = *++cmd; // fill 44
			case 0xac: pixels[y * p + x++] = *++cmd; // fill 43
			case 0xa8: pixels[y * p + x++] = *++cmd; // fill 42
			case 0xa4: pixels[y * p + x++] = *++cmd; // fill 41
			case 0xa0: pixels[y * p + x++] = *++cmd; // fill 40
			case 0x9c: pixels[y * p + x++] = *++cmd; // fill 39
			case 0x98: pixels[y * p + x++] = *++cmd; // fill 38
			case 0x94: pixels[y * p + x++] = *++cmd; // fill 37
			case 0x90: pixels[y * p + x++] = *++cmd; // fill 36
			case 0x8c: pixels[y * p + x++] = *++cmd; // fill 35
			case 0x88: pixels[y * p + x++] = *++cmd; // fill 34
			case 0x84: pixels[y * p + x++] = *++cmd; // fill 33
			case 0x80: pixels[y * p + x++] = *++cmd; // fill 32
			case 0x7c: pixels[y * p + x++] = *++cmd; // fill 31
			case 0x78: pixels[y * p + x++] = *++cmd; // fill 30
			case 0x74: pixels[y * p + x++] = *++cmd; // fill 29
			case 0x70: pixels[y * p + x++] = *++cmd; // fill 28
			case 0x6c: pixels[y * p + x++] = *++cmd; // fill 27
			case 0x68: pixels[y * p + x++] = *++cmd; // fill 26
			case 0x64: pixels[y * p + x++] = *++cmd; // fill 25
			case 0x60: pixels[y * p + x++] = *++cmd; // fill 24
			case 0x5c: pixels[y * p + x++] = *++cmd; // fill 23
			case 0x58: pixels[y * p + x++] = *++cmd; // fill 22
			case 0x54: pixels[y * p + x++] = *++cmd; // fill 21
			case 0x50: pixels[y * p + x++] = *++cmd; // fill 20
			case 0x4c: pixels[y * p + x++] = *++cmd; // fill 19
			case 0x48: pixels[y * p + x++] = *++cmd; // fill 18
			case 0x44: pixels[y * p + x++] = *++cmd; // fill 17
			case 0x40: pixels[y * p + x++] = *++cmd; // fill 16
			case 0x3c: pixels[y * p + x++] = *++cmd; // fill 15
			case 0x38: pixels[y * p + x++] = *++cmd; // fill 14
			case 0x34: pixels[y * p + x++] = *++cmd; // fill 13
			case 0x30: pixels[y * p + x++] = *++cmd; // fill 12
			case 0x2c: pixels[y * p + x++] = *++cmd; // fill 11
			case 0x28: pixels[y * p + x++] = *++cmd; // fill 10
			case 0x24: pixels[y * p + x++] = *++cmd; // fill 9
			case 0x20: pixels[y * p + x++] = *++cmd; // fill 8
			case 0x1c: pixels[y * p + x++] = *++cmd; // fill 7
			case 0x18: pixels[y * p + x++] = *++cmd; // fill 6
			case 0x14: pixels[y * p + x++] = *++cmd; // fill 5
			case 0x10: pixels[y * p + x++] = *++cmd; // fill 4
			case 0x0c: pixels[y * p + x++] = *++cmd; // fill 3
			case 0x08: pixels[y * p + x++] = *++cmd; // fill 2
			case 0x04: pixels[y * p + x++] = *++cmd; // fill 1
				continue;
			case 0xfd: pixels[y * p + x++] = 0; // skip 63
			case 0xf9: pixels[y * p + x++] = 0; // skip 62
			case 0xf5: pixels[y * p + x++] = 0; // skip 61
			case 0xf1: pixels[y * p + x++] = 0; // skip 60
			case 0xed: pixels[y * p + x++] = 0; // skip 59
			case 0xe9: pixels[y * p + x++] = 0; // skip 58
			case 0xe5: pixels[y * p + x++] = 0; // skip 57
			case 0xe1: pixels[y * p + x++] = 0; // skip 56
			case 0xdd: pixels[y * p + x++] = 0; // skip 55
			case 0xd9: pixels[y * p + x++] = 0; // skip 54
			case 0xd5: pixels[y * p + x++] = 0; // skip 53
			case 0xd1: pixels[y * p + x++] = 0; // skip 52
			case 0xcd: pixels[y * p + x++] = 0; // skip 51
			case 0xc9: pixels[y * p + x++] = 0; // skip 50
			case 0xc5: pixels[y * p + x++] = 0; // skip 49
			case 0xc1: pixels[y * p + x++] = 0; // skip 48
			case 0xbd: pixels[y * p + x++] = 0; // skip 47
			case 0xb9: pixels[y * p + x++] = 0; // skip 46
			case 0xb5: pixels[y * p + x++] = 0; // skip 45
			case 0xb1: pixels[y * p + x++] = 0; // skip 44
			case 0xad: pixels[y * p + x++] = 0; // skip 43
			case 0xa9: pixels[y * p + x++] = 0; // skip 42
			case 0xa5: pixels[y * p + x++] = 0; // skip 41
			case 0xa1: pixels[y * p + x++] = 0; // skip 40
			case 0x9d: pixels[y * p + x++] = 0; // skip 39
			case 0x99: pixels[y * p + x++] = 0; // skip 38
			case 0x95: pixels[y * p + x++] = 0; // skip 37
			case 0x91: pixels[y * p + x++] = 0; // skip 36
			case 0x8d: pixels[y * p + x++] = 0; // skip 35
			case 0x89: pixels[y * p + x++] = 0; // skip 34
			case 0x85: pixels[y * p + x++] = 0; // skip 33
			case 0x81: pixels[y * p + x++] = 0; // skip 32
			case 0x7d: pixels[y * p + x++] = 0; // skip 31
			case 0x79: pixels[y * p + x++] = 0; // skip 30
			case 0x75: pixels[y * p + x++] = 0; // skip 29
			case 0x71: pixels[y * p + x++] = 0; // skip 28
			case 0x6d: pixels[y * p + x++] = 0; // skip 27
			case 0x69: pixels[y * p + x++] = 0; // skip 26
			case 0x65: pixels[y * p + x++] = 0; // skip 25
			case 0x61: pixels[y * p + x++] = 0; // skip 24
			case 0x5d: pixels[y * p + x++] = 0; // skip 23
			case 0x59: pixels[y * p + x++] = 0; // skip 22
			case 0x55: pixels[y * p + x++] = 0; // skip 21
			case 0x51: pixels[y * p + x++] = 0; // skip 20
			case 0x4d: pixels[y * p + x++] = 0; // skip 19
			case 0x49: pixels[y * p + x++] = 0; // skip 18
			case 0x45: pixels[y * p + x++] = 0; // skip 17
			case 0x41: pixels[y * p + x++] = 0; // skip 16
			case 0x3d: pixels[y * p + x++] = 0; // skip 15
			case 0x39: pixels[y * p + x++] = 0; // skip 14
			case 0x35: pixels[y * p + x++] = 0; // skip 13
			case 0x31: pixels[y * p + x++] = 0; // skip 12
			case 0x2d: pixels[y * p + x++] = 0; // skip 11
			case 0x29: pixels[y * p + x++] = 0; // skip 10
			case 0x25: pixels[y * p + x++] = 0; // skip 9
			case 0x21: pixels[y * p + x++] = 0; // skip 8
			case 0x1d: pixels[y * p + x++] = 0; // skip 7
			case 0x19: pixels[y * p + x++] = 0; // skip 6
			case 0x15: pixels[y * p + x++] = 0; // skip 5
			case 0x11: pixels[y * p + x++] = 0; // skip 4
			case 0x0d: pixels[y * p + x++] = 0; // skip 3
			case 0x09: pixels[y * p + x++] = 0; // skip 2
			case 0x05: pixels[y * p + x++] = 0; // skip 1
				continue;
			}

			switch (command) {
			case 0x0f:
				i = w;
				break;
			case 0x0a:
				count = cmd_or_next(&cmd, 4);
				for (++cmd; count; --count) {
					pixels[y * p + x++] = *cmd + 0x10 * (player + 1);
				}
				dynamic = true;
				break;
			case 0x07:
				count = cmd_or_next(&cmd, 4);
				//dbgf("fill: %u pixels\n", count);
				for (++cmd; count; --count) {
					//dbgf(" %x", (unsigned)(*cmd) & 0xff);
					pixels[y * p + x++] = *cmd;
				}
				break;
			case 0x02:
				count = ((*cmd & 0xf0) << 4) + cmd[1];
				for (++cmd; count; --count)
					pixels[y * p + x++] = *++cmd;
				break;
			default:
				dbgf("unknown cmd at %X: %X, %X\n",
					 (unsigned)(cmd - (const unsigned char*)data),
					*cmd, command
				);
				// dump some more bytes
				for (size_t i = 0; i < 16; ++i)
					dbgf(" %02hhX", *((const unsigned char*)cmd + i));
				dbgs("");
				#if 1
				while (*cmd != 0xf)
					++cmd;
				i = w;
				#endif
				break;
			}
		}

		#if 0
		while (cmd[-1] != 0xf)
			++cmd;
		#endif
	}
#if DEBUG_SLP
	dbgs("");
#endif

	if (SDL_SetColorKey(surface, SDL_TRUE, 0))
		fprintf(stderr, "Could not set transparency: %s\n", SDL_GetError());

	return dynamic;
}
