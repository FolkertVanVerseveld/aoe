#ifndef GENIE_BKG_H
#define GENIE_BKG_H

#include "drs.h"

struct bkg {
	struct drs_fref tbl[3]; // FIXME figure out purpose
	struct drs_ref palette;
	struct drs_ref cursor;
	struct drs_ref shade;
	struct drs_ref button;
	struct drs_ref popup;
	unsigned position; // XXX not sure about signedness
	struct {
		unsigned index;
		unsigned char bevel[6];
		unsigned char text[2][3];
		unsigned char focus[2][3];
		unsigned char state[2][3];
	} col;
};

#endif
