/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef SCN_H
#define SCN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct scn_hdr {
	char version[4];
	uint32_t dword4, dword8;
	char typeC[4];
	uint32_t text_length;
	char text[];
};

int scn_read(struct scn *dst, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
