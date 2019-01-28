/* Copyright 2018-2019 Folkert van Verseveld. All rights reserved */

/**
 * Bitmap file header diagnostics tool.
 *
 * Licensed under Affero General Public License v3.0
 * Copyright Folkert van Verseveld
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "bmp.h"

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

/** Fetch color table. */
uint8_t *get_coltbl(const void *data, size_t size, unsigned bits)
{
	size_t n = 1 << bits;

	if (n * sizeof(uint32_t) > size) {
		fputs("corrupt color table\n", stderr);
		return NULL;
	}

	return (uint8_t*)((uint32_t*)data + n);
}

/** Print image header information. */
void dump_img(const void *data, size_t n, size_t offset)
{
	const struct img_header *hdr = data;

	if (n < 12 || hdr->biSize >= n) {
		fputs("corrupt img header\n", stderr);
		return;
	}

	printf("img header:\nbiSize: 0x%X\nbiWidth: %u\nbiHeight: %u\n", hdr->biSize, hdr->biWidth, hdr->biHeight);

	if (hdr->biSize < 12)
		return;

	printf("biPlanes: %u\nbiBitCount: %u\nbiCompression: %u\n", hdr->biPlanes, hdr->biBitCount, hdr->biCompression);
	printf("biSizeImage: 0x%X\n", hdr->biSizeImage);
	printf("biXPelsPerMeter: %u\nbiYPelsPerMeter: %u\n", hdr->biXPelsPerMeter, hdr->biYPelsPerMeter);
	printf("biClrUsed: %u\nbiClrImportant: %u\n", hdr->biClrUsed, hdr->biClrImportant);

	if (hdr->biBitCount <= 8) {
		uint8_t *end = get_coltbl((const unsigned char*)data + hdr->biSize, n - hdr->biSize, hdr->biBitCount);

		printf("pixel data start: 0x%zX\n", offset + hdr->biSize + 256 * sizeof(uint32_t));

		if (end && end + hdr->biHeight * hdr->biWidth > (const unsigned char*)data + n)
			fputs("truncated pixel data\n", stderr);
	}
}

/** Print general bitmap information. */
void dump_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;

	if (!is_bmp(data, n)) {
		if (!is_img(data, n))
			fputs("corrupt bmp file\n", stderr);
		else
			dump_img(data, n, 0);
		return;
	}

	printf("bmp header:\nbfType: 0x%04X\nSize: 0x%X\n", hdr->bfType, hdr->bfSize);
	printf("Start pixeldata: 0x%X\n", hdr->bfOffBits);
	dump_img(hdr + 1, n - sizeof *hdr, sizeof *hdr);
}

/** Read specified file completely and dump general information. */
int main(int argc, char **argv)
{
	FILE *f = NULL;
	long size;
	int ret = 1;
	char *data = NULL;

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "bmp");
		return 1;
	}

	f = fopen(argv[1], "rb");
	if (!f || fseek(f, 0, SEEK_END)) {
		perror(argv[1]);
		goto fail;
	}

	size = ftell(f);
	if (size < 0 || fseek(f, 0, SEEK_SET)) {
		perror(argv[1]);
		goto fail;
	}

	if (!(data = malloc(size)) || fread(data, size, 1, f) != 1) {
		perror(argv[1]);
		goto fail;
	}

	printf("bmp size: %lX (%ld)\n", size, size);
	dump_bmp(data, size);

	ret = 0;
fail:
	if (data)
		free(data);
	if (f)
		fclose(f);

	return ret;
}
