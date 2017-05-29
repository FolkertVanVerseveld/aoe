#ifndef GENIE_SHAPE_H
#define GENIE_SHAPE_H

#include <stddef.h>
#include <stdint.h>

#define SHAPE_V1_0  0x312e3030
#define SHAPE_V1_1  0x312e3031
#define SHAPE_V1_2  0x312e3032
#define SHAPE_V1_3  0x312e3033
#define SHAPE_V1_4  0x312e3034
#define SHAPE_V1_5  0x312e3035
#define SHAPE_V1_6  0x312e3036
#define SHAPE_V1_7  0x312e3037
#define SHAPE_V1_8  0x312e3038
#define SHAPE_V1_9  0x312e3039
#define SHAPE_V1_10 0x312e3130
#define SHAPE_V1_11 0x312e3131
#define SHAPE_V1_12 0x312e3132

struct shape {
	uint32_t magic;
	int32_t vertices;
	uint32_t pad[2];
	const char *data;
};

int shape_read(const void *data, struct shape *shape, size_t n);

#endif
