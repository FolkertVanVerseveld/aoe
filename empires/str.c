/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include "str.h"

#include <string.h>

void astr16_str(char *buf, size_t bufsz, const struct astr16 *str)
{
	if (str->length > bufsz) {
		strncpy(buf, str->data, bufsz);
		buf[bufsz - 1] = '\0';
	} else {
		strncpy(buf, str->data, str->length);
		buf[str->length] = '\0';
	}
}

size_t astr16_size(const struct astr16 *str)
{
	return sizeof *str + str->length;
}

struct astr16 *astr16_next(const struct astr16 *str)
{
	return (struct astr16*)((unsigned char*)str + astr16_size(str));
}
