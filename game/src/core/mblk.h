#ifndef MBLK_H
#define MBLK_H 1

/** Memory BLocK to ease safe memory and string manipulation */

#include <stddef.h>  // size_t
#include <stdbool.h> // bool
#include <string.h>  // strlen

#if __cplusplus
extern "C" {
#endif

// Memory Types
// Memory data nor memory block can be modified.
// Useful when mblk is defined in readonly section
#define MT_FIXED    0
// readonly memory, do not call free
#define MT_CONST    1
#define MT_ALLOC    2
#define MT_MASK  0x03

// Memory data is a str or const str
#define MT_STR 0x04
// Memory block is '\0' terminated (C string)
// NOTE: when set, len is excluding '\0' byte
// not set for Pascal/C++ strings
#define MT_NUL 0x08

#define MT_C_STRA (MT_ALLOC | MT_STR | MT_NUL)

// Memory Block return codes
#define MB_OK     0
// (re)allocation failed
#define MB_NOMEM  1
// mblk is in readonly memory and cannot be modified
#define MB_FIXED  2
// resize is not allowed
#define MB_CONST  3
// resize would cause numeric overflow
#define MB_TOOBIG 4
// cannot shrink empty string
#define MB_EMPTY  5
// new size would make string bigger
#define MB_BAD_SHRINK 6
// pointer to shrink mblk is out of bounds
#define MB_BAD_SHRINK_PTR 7
// TODO add missing codes
// string to initialise is malformed: `len != 0 && str == NULL`
#define MB_BAD_INIT_STR   9

/** Memory BLocK to ease safe memory and string manipulation */
struct mblk {
	unsigned flags;
	union mptr {
		char *str;        // mutable  text
		const char *cstr; // readonly text
		void *mem;        // mutable  blob
		const void *cmem; // readonly blob
	} buf;
	size_t len; // LENgth. NOTE: for `flags & MT_NUL`:
	            // `len + 1` is real length iff `cap > 0`
	size_t cap; // CAPacity
};

#define MBLK_INIT {0}

static inline void mblk_init(struct mblk *m, unsigned flags, const void *ptr, size_t len, size_t cap)
{
	m->flags = flags;
	m->buf.cmem = ptr;
	m->len = len;
	m->cap = cap;
}

void mblk_free(struct mblk *m);

static inline bool mblk_is_fixed(const struct mblk *m)
{
	return (m->flags & MT_MASK) == MT_FIXED;
}

static inline bool mblk_is_alloc(const struct mblk *m)
{
	return (m->flags & MT_MASK) == MT_ALLOC;
}

int mblk_try_cap(struct mblk *m, size_t newcap);
int mblk_try_grow(struct mblk *m, size_t more);

/**
 * Set mblk to C string with precomputed len.
 * Assumes mblk has been initialised already.
 */
int mstr_const_set_len0(struct mblk *s, const char *str, size_t len);

static inline int mstr_const_set0(struct mblk *s, const char *str)
{
	return mstr_const_set_len0(s, str, strlen(str));
}

int mstr_init_len0(struct mblk *s, const char *str, size_t len);

static inline int mstr_init0(struct mblk *s, const char *str)
{
	return mstr_init_len0(s, str, str ? strlen(str) : 0);
}

/** Shallow copy (like structure assignment). */
static inline void mstr_cpy(struct mblk *dst, const struct mblk *src)
{
	dst->flags = src->flags;
	dst->buf.cmem = src->buf.cmem;
	dst->len = src->len;
	dst->cap = src->cap;
}

int mstr_bpop(struct mblk *s); /** Remove last character */
char *mstr_rchr(const struct mblk *s, int c); /** Like strrchr */

int mstr_shrink(struct mblk *s, size_t size);

static inline int mstr_shrink_ptr(struct mblk *s, const char *ptr)
{
	if (ptr < s->buf.cstr)
		return MB_BAD_SHRINK_PTR;

	size_t size = (size_t)(ptrdiff_t)(ptr - s->buf.cstr);
	return mstr_shrink(s, size);
}

/** Memory STRing APPEND in REVerse. */
int mstr_append_rev(struct mblk *s, const char *str);

/** Append CHaracter to end of \a s. */
int mstr_addch(struct mblk *s, int ch);

int mstr_addstr_len(struct mblk *s, const char *str, size_t len);

static inline int mstr_addstr(struct mblk *s, const char *str)
{
	return mstr_addstr_len(s, str, strlen(str));
}

#if __cplusplus
}
#endif

#endif
