#include "drs.h"
#include <stdio.h>

#define stre(expr) #expr
#define str(expr) stre(expr)

int drs_ref_read(const char *data, struct drs_ref *ref)
{
	return sscanf(data, "%" str(DRS_REF_NAMESZ) "s %u", ref->name, &ref->id) != 2;
}

int drs_fref_read(const char *data, struct drs_fref *ref)
{
	return sscanf(data,
		"%" str(DRS_REF_NAMESZ) "s "
		"%" str(DRS_REF_NAMESZ) "s %u %u",
		ref->name, ref->name2, &ref->id, &ref->id2
	) != 4;
}
