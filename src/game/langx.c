#include <stdio.h>
#include <string.h>
#include "langx.h"
#include <genie/rsrc.h>

int loadstr(unsigned id, char *str, unsigned n)
{
	// naively scan language files for UTF16 string
	unsigned i, tn;
	for (i = 0, tn = strtbl.n; i < tn; ++i) {
		struct rstrptr *ptr = &strtbl.a[i];
		if (ptr->id == id) {
			struct rsrcstr *rs = &ptr->str;
			uint16_t j, sn = rs->length;
			if (sn > n)
				sn = n;
			const char *s = rs->str;
			for (j = 0; j < sn; s += 2)
				str[j++] = *s;
			if (n)
				str[n - 1] = '\0';
			return 1;
		}
	}
	fprintf(stderr, "bad str: %u\n", id);
	return 0;
}
