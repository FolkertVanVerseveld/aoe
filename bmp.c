#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

struct bmp_header {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} __attribute__((packed));

struct img_header {
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXPelsPerMeter;
	uint32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};

int is_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	return n >= sizeof *hdr && hdr->bfType == 0x4D42;
}

int is_img(const void *data, size_t n)
{
	const struct img_header *hdr = data;
	return n >= 12 && hdr->biSize < n;
}

uint8_t *dump_coltbl(const void *data, size_t size, unsigned bits)
{
	size_t n = 1 << bits;
	if (n * sizeof(uint32_t) > size) {
		fputs("corrupt color table\n", stderr);
		return NULL;
	}
	uint32_t *col = data;
	for (size_t i = 0; i < n; ++i)
		printf("%8X: %8X\n", i, col[i]);
	return (uint8_t*)col[n];
}

void dump_img(const void *data, size_t n)
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
		uint8_t *end = dump_coltbl((const unsigned char*)data + hdr->biSize, n - hdr->biSize, hdr->biBitCount);
		if (end && end + hdr->biHeight * hdr->biWidth > (const unsigned char*)data + n)
			fputs("truncated pixel data\n", stderr);
	}
}

void dump_bmp(const void *data, size_t n)
{
	const struct bmp_header *hdr = data;
	if (!is_bmp(data, n)) {
		if (!is_img(data, n))
			fputs("corrupt bmp file\n", stderr);
		else
			dump_img(data, n);
		return;
	}
	printf("bmp header:\nbfType: 0x%04X\nSize: 0x%X\n", hdr->bfType, hdr->bfSize);
	printf("Start pixeldata: 0x%X\n", hdr->bfOffBits);
	dump_img(hdr + 1, n - sizeof *hdr);
}

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
