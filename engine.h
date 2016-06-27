#ifndef AOE_ENGINE_H
#define AOE_ENGINE_H

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

#endif
