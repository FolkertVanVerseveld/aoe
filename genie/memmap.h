#ifndef GENIE_MEMMAP_H
#define GENIE_MEMMAP_H

#include <stddef.h>

void meminit(void);
void memchk(void);
#ifdef DEBUG
void memstat(void);
#else
#define memstat() ((void)0)
#endif
void memfree(void);

void *vnew(size_t n, const char *file, const char *func, size_t lineno, const char *desc);
#define new2(n,desc) vnew(n,__FILE__,__func__,__LINE__,desc)
#define new(n) vnew(n,__FILE__,__func__,__LINE__,NULL)
void nuke(void *ptr, const char *file, const char *func, size_t lineno, const char *desc);
#define delete2(p,desc) do{nuke(p,__FILE__,__func__,__LINE__,desc);p=NULL;}while(0)
#define delete(p) do{nuke(p,__FILE__,__func__,__LINE__,NULL);p=NULL;}while(0)

#endif
