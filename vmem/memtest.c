#include <stdlib.h>
#include "memmap.h"

int main(void)
{
	int *a = NULL, *b = NULL;
	meminit();
	a = new(sizeof(int));
	*a = 9;
	b = new(sizeof(int));
	b[0] = 1;
	memstat();
	delete(a);
	delete(b);
	memchk();
	memfree();
	return 0;
}
