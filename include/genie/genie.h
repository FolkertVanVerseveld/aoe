/* Copyright 2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef EMPIRES_H
#define EMPIRES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cfg.h"

// maximum resolution supported by original game
#define MAX_BKG_WIDTH 1024
#define MAX_BKG_HEIGHT 768

void open_readme(void);
void video_play(const char *name);

int GE_Init(int *pargc, char **argv);
int GE_Quit(void);

#ifdef __cplusplus
}
#endif

#endif
