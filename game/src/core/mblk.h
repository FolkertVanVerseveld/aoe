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
// TODO add missing codes
// string to initialise is malformed: `len != 0 && str == NULL`
#define MB_BAD_INIT_STR 9

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

#if __cplusplus
}
#endif

#endif
