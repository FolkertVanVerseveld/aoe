/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * PE resource wrapper API
 *
 * peres.c portions are licensed under General Public License v3.0
 */

#include <genie/res.h>

#include <genie/bmp.h>
#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/memory.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pe.h"

static void *scratch;
static size_t scratch_size;

#define PE_DATA_ENTRY_NAMESZ 64

struct pe_data_entry {
       char name[PE_DATA_ENTRY_NAMESZ];
       unsigned type;
       unsigned offset;
       unsigned id;
       void *data;
       unsigned size;
};

static size_t nextpow2(size_t v)
{
       --v;
       v |= v >> 1;
       v |= v >> 2;
       v |= v >> 4;
       v |= v >> 8;
       v |= v >> 16;
       v |= v >> 32;
       return ++v;
}

static void scratch_resize(size_t n)
{
       void *ptr;

       if (scratch && scratch_size >= n)
               return;

       n = nextpow2(n);
       scratch = mem_realloc(scratch, n);
       scratch_size = n;
}

int pe_lib_open(struct pe_lib *lib, const char *name)
{
	lib->pe = mem_alloc(sizeof(struct pe));
	pe_init(lib->pe);
	return pe_open(lib->pe, name);
}

void pe_lib_close(struct pe_lib *lib)
{
	pe_close(lib->pe);
	mem_free(lib->pe);
}

// NOTE strictly assumes file is at least 40 bytes
uint32_t img_pixel_offset(const void *data, size_t n)
{
	const struct img_header *hdr = data;

	if (n < 40 || hdr->biSize >= n || hdr->biBitCount != 8)
		return 0;

	// assume all entries are used
	return hdr->biSize + 256 * sizeof(uint32_t);
}

// XXX copied from bmp.c

/** Validate bitmap header. */
int is_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	return n >= sizeof *hdr && hdr->bfType == BMP_BF_TYPE;
}

/** Validate bitmap image header. */
int is_img(const void *data, size_t n)
{
	const struct img_header *hdr = data;
	return n >= 12 && hdr->biSize < n;
}

// END copied from bmp.c

/** Fetch color table. */
static uint8_t *get_coltbl(const void *data, size_t size, unsigned bits)
{
	size_t n = 1 << bits;
	if (n * sizeof(uint32_t) > size) {
		fputs("corrupt color table\n", stderr);
		return NULL;
	}
	return (uint8_t*)((uint32_t*)data + n);
}

static void dump_img(const void *data, size_t n, size_t offset)
{
	const struct img_header *hdr = data;
	if (n < 12 || hdr->biSize >= n) {
		fputs("corrupt img header\n", stderr);
		return;
	}
	dbgf("img header:\nbiSize: 0x%X\nbiWidth: %u\nbiHeight: %u\n", hdr->biSize, hdr->biWidth, hdr->biHeight);
	if (hdr->biSize < 12)
		return;
	dbgf("biPlanes: %u\nbiBitCount: %u\nbiCompression: %u\n", hdr->biPlanes, hdr->biBitCount, hdr->biCompression);
	dbgf("biSizeImage: 0x%X\n", hdr->biSizeImage);
	dbgf("biXPelsPerMeter: %u\nbiYPelsPerMeter: %u\n", hdr->biXPelsPerMeter, hdr->biYPelsPerMeter);
	dbgf("biClrUsed: %u\nbiClrImportant: %u\n", hdr->biClrUsed, hdr->biClrImportant);
	if (hdr->biBitCount <= 8) {
		uint8_t *end = get_coltbl((const unsigned char*)data + hdr->biSize, n - hdr->biSize, hdr->biBitCount);
		dbgf("pixel data start: 0x%zX\n", offset + hdr->biSize + 256 * sizeof(uint32_t));
		if (end && end + hdr->biHeight * hdr->biWidth > (const unsigned char*)data + n)
			fputs("truncated pixel data\n", stderr);
	}
}

static void dump_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	if (!is_bmp(data, n)) {
		if (!is_img(data, n))
			fputs("corrupt bmp file\n", stderr);
		else
			dump_img(data, n, 0);
		return;
	}
	dbgf("bmp header:\nbfType: 0x%04X\nSize: 0x%X\n", hdr->bfType, hdr->bfSize);
	dbgf("Start pixeldata: 0x%X\n", hdr->bfOffBits);
	dump_img(hdr + 1, n - sizeof *hdr, sizeof *hdr);
}

int load_string(struct pe_lib *lib, unsigned id, char *str, size_t size)
{
	pe_load_string(lib->pe, id, str, size);
	return 0;
}

// FIXME also support big endian machines
int load_bitmap(struct pe_lib *lib, unsigned id, void **data, size_t *size)
{
	struct pe_data_entry entry;
	struct bmp_header *hdr;

	dbgf("load_bitmap %04X\n", id);

	void *ptr;
	size_t count;

	if (pe_load_res(lib->pe, (unsigned)RT_BITMAP, id, &ptr, &count)) {
		dbgs("load_bitmap failed");
		return 1;
	}

	scratch_resize(count + sizeof *hdr);
	// reconstruct bmp_header in scratch mem
	uint32_t offset = img_pixel_offset(ptr, count);
	if (!offset) {
		dbgs("load_bitmap: bogus offset");
		return 1;
	}

	hdr = scratch;
	hdr->bfType = BMP_BF_TYPE;
	hdr->bfSize = entry.size + sizeof *hdr;
	hdr->bfReserved1 = hdr->bfReserved2 = 0;
	hdr->bfOffBits = offset + sizeof *hdr;

	memcpy(hdr + 1, ptr, count);
	dump_bmp(hdr, hdr->bfSize);

	*data = hdr;
	*size = count + sizeof *hdr;

	return 0;
}
