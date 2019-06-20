/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef CPN_H
#define CPN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define CPN_HDR_NAME_MAX 256
#define CPN_SCN_DESCRIPTION_MAX 255
#define CPN_SCN_FILENAME_MAX 257

struct cpn_hdr {
	char version[4];
	// note that the original game may leave junk in here after the null terminator
	char name[CPN_HDR_NAME_MAX];
	uint32_t scenario_count;
};

struct cpn_scn {
	char data0[8];
	char description[CPN_SCN_DESCRIPTION_MAX];
	char filename[CPN_SCN_FILENAME_MAX];
};

#define CPN_ERR_OK 0
#define CPN_ERR_BAD_HDR 1

struct cpn {
	struct cpn_hdr *hdr;
	struct cpn_scn *scenarios;
};

void cpn_init(struct cpn *c);
void cpn_free(struct cpn *c);
int cpn_read(struct cpn *c, const void *data, size_t size);
size_t cpn_list_size(const struct cpn *c);

#ifdef __cplusplus
}
#endif

#endif
