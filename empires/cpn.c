/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stddef.h>

#include "../empires/cpn.h"

void cpn_init(struct cpn *c)
{
	c->hdr = NULL;
}

void cpn_free(struct cpn *c)
{
	#if DEBUG
	cpn_init(c);
	#endif
}

int cpn_read(struct cpn *dst, const void *data, size_t size)
{
	if (size < sizeof(struct cpn_hdr))
		return CPN_ERR_BAD_HDR;

	dst->hdr = (void*)data;
	// TODO add more bounds checking
	dst->scenarios = (struct cpn_scn*)&dst->hdr[1];

	return 0;
}

size_t cpn_list_size(const struct cpn *c)
{
	return sizeof(struct cpn) + c->hdr->scenario_count * sizeof(struct cpn_scn);
}
