#ifndef AOE_MAP_H
#define AOE_MAP_H

struct map_0 {
	void *ptr14;
};

#define MAPBLKSZ 108

struct map {
	struct map_0 *obj0;
	unsigned num4, num8;
	void *ptrC;
	unsigned num10, num14, num18, num1C, num20, num24, num28, num2C, num30, num34;
	unsigned hdc38;
	unsigned num3C;
	char blk40[MAPBLKSZ];
	void *ptrAC;
	char *name;
	unsigned numB4;
	void *ptrBC;
	unsigned numC0;
	void *ptrC4;
	unsigned tblC8[3];
	unsigned numD4, numD8, numDC, numE0, numE4, numE8, numEC, numF0, numF4, numF8;
	char chFC, padFD[3];
};

struct map *map_init(struct map *this, const char *map_name, int a2);
void map_free(struct map *this);

#endif
