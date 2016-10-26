#ifndef AOE_ENGINE_H
#define AOE_ENGINE_H

#include <stddef.h>

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
void eng_free(void);

void show_error(const char *title, const char *msg);

struct game_drive {
	unsigned driveno;
	char buf[256];
	unsigned driveno2;
	char buf2[64];
	char gap[300];
};

struct game_drive *game_drive_init(struct game_drive *this);

#endif
