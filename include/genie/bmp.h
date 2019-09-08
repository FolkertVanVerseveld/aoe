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

#ifndef _WIN32
	#ifndef _DEFAULT_SOURCE
		#define _DEFAULT_SOURCE
	#endif
	#ifndef _BSD_SOURCE
		#define _BSD_SOURCE
	#endif
	#include <endian.h>
#else
	#ifndef __BYTE_ORDER__
		#error unknown byte order
	#endif

	// byte swapping stuff
	#include <stdlib.h>

	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define be16toh(v) (v)
		#define be32toh(v) (v)
		#define be64toh(v) (v)

		#define le16toh(v) _byteswap_ushort(v)
		#define le32toh(v) _byteswap_ulong(v)
		#define le64toh(v) _byteswap_uint64(v)
	#else
		// how can micro$oft fuck this up...
		#define be16toh(v) _byteswap_ushort(v)
		#define be32toh(v) _byteswap_ulong(v)
		#define be64toh(v) _byteswap_uint64(v)

		#define le16toh(v) (v)
		#define le32toh(v) (v)
		#define le64toh(v) (v)
	#endif

	#define htobe16(v) be16toh(v)
	#define htobe32(v) be32toh(v)
	#define htobe64(v) be64toh(v)

	#define htole16(v) le16toh(v)
	#define htole32(v) le32toh(v)
	#define htole64(v) le64toh(v)
#endif

#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	#define BMP_BF_TYPE 0x424D
#else
	#define BMP_BF_TYPE 0x4D42
#endif

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
