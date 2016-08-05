#ifndef AOE_TODO_H
#define AOE_TODO_H

#include <stdio.h>
#include <stdlib.h>

#define stub fprintf(stderr,"stub: %s\n",__func__);
#define halt() do{fputs("can't continue, program halted\n",stderr);exit(1);}while(0)

#endif
