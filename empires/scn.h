/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef SCN_H
#define SCN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct scn_hdr {
	char version[4];
	uint32_t dword4, dword8;
	uint32_t time;
	uint32_t instructions_length;
	char instructions[];
	//uint32_t dword14; // unknown constant, usually 0 or 1
	//uint32_t player_count;
};

#define SCN_ERR_OK 0
#define SCN_ERR_BAD_HDR 1

struct scn {
	struct scn_hdr *hdr;
	void *data;
	size_t size;
};

void scn_init(struct scn *s);
void scn_free(struct scn *s);
int scn_read(struct scn *dst, const void *data, size_t size);

#ifdef __cplusplus
}
#endif

#endif
