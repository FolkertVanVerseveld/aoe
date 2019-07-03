/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef STR_H
#define STR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct astr16 {
	uint16_t length;
	char data[];
};

void astr16_str(char *buf, size_t bufsz, const struct astr16 *str);
size_t astr16_size(const struct astr16 *str);
struct astr16 *astr16_next(const struct astr16 *str);

#ifdef __cplusplus
}
#endif

#endif
