/* Copyright 2018-2019 Folkert van Verseveld. All rights reserved */

/**
 * Provides Bitmap image file headers.
 *
 * Licensed under Affero General Public License v3.0
 * Copyright Folkert van Verseveld.
 *
 * BMP file format source: http://www.dragonwins.com/domains/GetTechEd/bmp/bmpfileformat.htm
 */

#ifndef BMP_H
#define BMP_H

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <endian.h>

#include <stdint.h>

#define BMP_BF_TYPE be16toh(0x424D)

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

#endif
