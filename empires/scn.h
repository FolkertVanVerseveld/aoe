/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef SCN_H
#define SCN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct scn_hdr {
	char version[4];
	uint32_t dword4, dword8;
	uint32_t time;
	uint32_t instructions_length;
	char instructions[];
};

struct scn_hdr2 {
	// this struct continues where scn_hdr ends
	uint32_t dword14; // usually 0 or 1
	uint32_t player_count;
};

struct scn_hdr3 {
	// this struct continues where scn_hdr2 ends
	// NOTE the following data has already been inflated
	uint8_t byte0[0x1111];
	uint16_t filename_length;
	char filename[];
} __attribute__((packed));

struct scn_description {
	// this struct continues where scn_hdr3 ends
	uint16_t description_length;
	char description[];
};

// FIXME bogus structure for some scenarios
struct scn_global_victory {
	uint8_t type;
	uint16_t data;
} __attribute__((packed));

#define SCN_ERR_OK 0
#define SCN_ERR_BAD_HDR 1
#define SCN_ERR_INFLATE 2
#define SCN_ERR_DEFLATE 3

struct scn {
	struct scn_hdr *hdr;
	struct scn_hdr2 *hdr2;
	struct scn_hdr3 *hdr3;
	void *data, *inflated, *deflated;
	size_t size, inflated_size, deflated_size;
};

void scn_init(struct scn *s);
void scn_free(struct scn *s);
int scn_read(struct scn *dst, const void *data, size_t size);
int scn_inflate(struct scn *s);
int scn_deflate(struct scn *s);

struct scn_description *scn_get_description(const struct scn *s);

#ifdef __cplusplus
}
#endif

#endif
