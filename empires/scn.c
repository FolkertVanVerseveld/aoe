/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "scn.h"
#include "serialization.h"

#include <assert.h>

void scn_init(struct scn *s)
{
	s->hdr = NULL;
	s->hdr2 = NULL;
	s->hdr3 = NULL;
	s->data = s->inflated = s->deflated = NULL;
	s->size = s->inflated_size = s->deflated_size = 0;
}

void scn_free(struct scn *s)
{
	free(s->inflated);
	free(s->deflated);
#if DEBUG
	scn_init(s);
#endif
}

int scn_read(struct scn *dst, const void *map, size_t size)
{
	if (size < sizeof(struct scn_hdr))
		return SCN_ERR_BAD_HDR;

	dst->hdr = (void*)map;
	// TODO add more bounds checking
	ptrdiff_t offdata = sizeof(struct scn_hdr) + dst->hdr->instructions_length;
	dst->hdr2 = (unsigned char*)map + offdata;
	dst->data = &dst->hdr2[1];
	dst->size = size;

	return 0;
}

int scn_inflate(struct scn *s)
{
	int err;
	size_t offset;

	assert(!s->inflated);
	offset = (const unsigned char*)s->data - (const unsigned char*)s->hdr;

	if ((err = raw_inflate(&s->inflated, &s->inflated_size, s->data, s->size - offset)))
		return err;

	s->hdr3 = s->inflated;
	return 0;
}
