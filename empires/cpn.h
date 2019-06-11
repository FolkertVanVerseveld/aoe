/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef CPN_H
#define CPN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct cpn_hdr {
	char version[4];
	// note that the original game may leave junk in here after the null terminator
	char name[256];
	uint32_t scenario_count;
};

struct cpn_scn {
	char data0[8];
	char description[255];
	char filename[257];
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

#ifdef __cplusplus
}
#endif

#endif
