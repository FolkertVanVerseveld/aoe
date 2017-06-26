/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#ifndef AOE_ENGINE_H
#define AOE_ENGINE_H

#include <stddef.h>
#include <genie/prompt.h>

extern int fd_langx, fd_lang;
extern char *data_langx, *data_lang;
extern size_t size_langx, size_lang;

struct regpair;

union regparent {
	// REMAP typeof(HKEY *key) == void**
	unsigned key;
	struct regpair *root;
};

union reg_value {
	int dword;
	short word;
	char byte;
	void *ptr;
};

struct regpair {
	union regparent left;
	union reg_value value;
	struct regpair *other;
};

int findfirst(const char *fname);
int loadlib(const char *name, char **data, size_t *size);
void libdump(void);

int eng_init(char **path);
int eng_main(void);
void eng_free(void);

struct game_drive {
	unsigned driveno;
	char buf[256];
	unsigned driveno2;
	char buf2[64];
	char gap[300];
};

struct game_drive *game_drive_init(struct game_drive *this);

#endif
