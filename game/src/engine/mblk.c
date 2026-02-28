#include <mblk.h>

#include <assert.h>
#include <stdlib.h>
#include <limits.h>

/** Try free if MT_ALLOC is set */
void mblk_free(struct mblk *m)
{
	if (mblk_is_alloc(m))
		free(m->buf.mem);
}

int mstr_const_set_len0(struct mblk *s, const char *str, size_t len)
{
	assert(len != SIZE_MAX);

	if (mblk_is_fixed(s))
		return MB_FIXED; // cannot modify m

	mblk_free(s); // does nothing if MT_ALLOC not set

	s->flags = MT_CONST | MT_STR | MT_NUL;
	s->buf.cstr = str;
	s->len = len;
	s->cap = len + 1;

	return MB_OK;
}

int mstr_init_len0(struct mblk *s, const char *str, size_t len)
{
	if (!str) {
		if (len)
			return MB_BAD_INIT_STR;

		mblk_init(s, MT_C_STRA, NULL, 0, 0);
		return MB_OK;
	}

	assert(len != SIZE_MAX); // len + 1 would overflow
	char *cpy = malloc(len + 1);

	if (!cpy)
		return MB_NOMEM;

	memcpy(cpy, str, len + 1);
	mblk_init(s, MT_C_STRA, cpy, len, len + 1);

	return MB_OK;
}