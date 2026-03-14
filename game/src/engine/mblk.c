#include <mblk.h>

#include <assert.h>
#include <stdlib.h> // realloc, free
#include <limits.h> // SIZE_MAX

// inspired by: https://github.com/esmil/musl/blob/master/src/string/memrchr.c
// copyright libmusl Rich Felker
/** Substitute for memrchr to not rely on GNU stuff */
static void *my_memrchr(const void *m, int c, size_t n)
{
	const unsigned char *s = m;
	c = (unsigned char)c;

	while (n --> 0)
		if (s[n] == c)
			return (void*)(s + n);

	return NULL;
}

/** Try free if MT_ALLOC is set */
void mblk_free(struct mblk *m)
{
	void *ptr;

	if (mblk_is_alloc(m) && (ptr = m->buf.mem) != NULL)
		free(ptr); // free also accepts NULL,
		           // but avoid calling when we don't have to
}

int mstr_const_set_len0(struct mblk *s, const char *str, size_t len)
{
	assert(len != SIZE_MAX); // len + 1 would overflow

	if (mblk_is_fixed(s))
		return MB_FIXED; // cannot modify m

	mblk_free(s); // does nothing if MT_ALLOC not set
	mblk_init(s, MT_CONST | MT_STR | MT_NUL, str, len, len + 1);

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

	memcpy(cpy, str, len);
	cpy[len] = '\0';
	mblk_init(s, MT_C_STRA, cpy, len, len + 1);

	return MB_OK;
}

int mstr_bpop(struct mblk *s)
{
	if (!mblk_is_alloc(s))
		return MB_CONST;

	if (!s->len)
		return MB_EMPTY;

	size_t newlen = --s->len;

	if (s->flags & MT_NUL)
		s->buf.str[newlen] = '\0';

	return MB_OK;
}

char *mstr_rchr(const struct mblk *s, int c)
{
	if ((unsigned char)c == '\0') {
		// only allowed for '\0' terminated strings
		// as pointer will be past end for non C strings
		assert(s->flags & MT_NUL);
		return &s->buf.str[s->len];
	}

	size_t len = s->len;

	// XXX do we need this? we already check for '\0'...
	// this will only be useful if the '\0' byte has been overwritten
	if (s->flags & MT_NUL)
		++len;

	return my_memrchr(s->buf.cstr, c, len);
}

int mstr_shrink(struct mblk *s, size_t size)
{
	if (!mblk_is_alloc(s))
		return MB_CONST;

	if (s->flags & MT_NUL) {
		if (size == SIZE_MAX)
			return MB_TOOBIG;

		if (size + 1 > s->len)
			return MB_BAD_SHRINK;

		s->buf.str[s->len = size] = '\0';
	} else {
		if (size > s->len)
			return MB_BAD_SHRINK;

		s->len = size;
	}

	return MB_OK;
}

int mstr_append_rev(struct mblk *s, const char *str)
{
	// no need to check for mblk_is_alloc: mblk_try_grow does this
	if (!*str)
		return MB_OK;

	size_t len = strlen(str);
	int ret = mblk_try_grow(s, len);

	if (ret != MB_OK)
		return ret;

	size_t newlen = s->len;
	char *dst = &s->buf.str[newlen];

	// TODO consider writing memrcpy?
	for (size_t i = len; i --> 0;)
		*dst++ = str[i];

	if (s->flags & MT_NUL)
		*dst = '\0';

	s->len = newlen += len;

	return MB_OK;
}

int mblk_try_cap(struct mblk *m, size_t newcap)
{
	if (!mblk_is_alloc(m))
		return MB_CONST;

	// accomodate for optional '\0' byte
	if (m->flags & MT_NUL) {
		if (newcap == SIZE_MAX)
			return MB_TOOBIG;

		++newcap;
	}

	// avoid realloc when we have the capacity
	if (newcap <= m->cap) {
		m->cap = newcap;
		return MB_OK;
	}

	void *ptr = realloc(m->buf.mem, newcap);
	if (!ptr)
		return MB_NOMEM;

	m->buf.mem = ptr;
	m->cap = newcap;

	return MB_OK;
}

int mblk_try_grow(struct mblk *m, size_t more)
{
	if (m->cap > SIZE_MAX - more)
		return MB_TOOBIG;

	return mblk_try_cap(m, m->len + more);
}

int mstr_addch(struct mblk *s, int ch)
{
	if (!mblk_is_alloc(s))
		return MB_CONST;

	int ret = mblk_try_grow(s, 1);
	if (ret != MB_OK)
		return ret;

	size_t len = s->len;
	char *str = s->buf.str;

	str[len] = ch;

	// TODO unit test this
	if (s->flags & MT_NUL)
		str[len + 1] = '\0';

	++s->len;
	return ret;
}

int mstr_addstr_len(struct mblk *s, const char *str, size_t len)
{
	if (!mblk_is_alloc(s))
		return MB_CONST;

	int ret = mblk_try_grow(s, len);
	if (ret != MB_OK)
		return ret;

	// TODO unit test this
	size_t oldlen = s->len, add = len;
	if (s->flags & MT_NUL)
		++add;

	memcpy(&s->buf.str[oldlen], str, add);
	s->len = oldlen + len;

	return MB_OK;
}