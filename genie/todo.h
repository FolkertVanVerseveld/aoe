#ifndef GENIE_TODO_H
#define GENIE_TODO_H

#include <stdio.h>
#include <stdlib.h>
#include "prompt.h"

#define stub fprintf(stderr,"stub: %s\n",__func__);
#define ORIGDROP
#define halt() do{char buf[256];snprintf(buf,sizeof buf,"%s: can't continue, not implemented yet\n",__func__);fputs(buf, stderr);show_message("Fatal error",buf,BTN_OK,MSG_ERR,BTN_OK);exit(1);}while(0)

#endif
