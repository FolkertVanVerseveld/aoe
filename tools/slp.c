/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct slp_frame_info {
	uint32_t ctbladdr;
	uint32_t otbladdr;
	uint32_t paladdr;
	uint32_t prop;
	uint32_t width;
	uint32_t height;
	int32_t hsx;
	int32_t hsy;
};

struct slp_row {
	int16_t left;
	int16_t right;
};

struct slp {
	char version_name[4];
	uint32_t num_frames;
	char comment[24];
};

/** copies from src to dest and sets (char*)dest + n = '\0' */
static void *null_memcpy(void *dest, const void *src, size_t n)
{
	void *ptr = memcpy(dest, src, n);
	*((char*)dest + n) = '\0';
	return ptr;
}

static struct slp_frame_info *get_frame(const void *data, unsigned frame)
{
	const struct slp_frame_info *table;
	table = (const struct slp_frame_info*)((char*)data + sizeof(struct slp));
	return (struct slp_frame_info*)&table[frame];
}

static void slp_frame_info_dump(const struct slp_frame_info *f)
{
	printf("command table offset: %" PRIx32 "\n", f->ctbladdr);
	printf("outline table offset: %" PRIx32 "\n", f->otbladdr);
	printf("palette offset: %" PRIx32 "\n", f->paladdr);
	printf("properties: %" PRIx32 "\n", f->prop);
	printf("width: %" PRIu32 "\n", f->width);
	printf("height: %" PRIu32 "\n", f->height);
	printf("hotspot x: %" PRId32 "\n", f->hsx);
	printf("hotspot y: %" PRId32 "\n", f->hsy);
}

static int slp_dump(const void *data, size_t size)
{
	const struct slp *slp = data;
	char buf[80];
	size_t offset = 0;

	if (size < sizeof *slp)
		return -1;

	null_memcpy(buf, slp->version_name, sizeof slp->version_name);
	printf("version: %s\n", buf);
	null_memcpy(buf, slp->comment, sizeof slp->comment);
	printf("comment: %s\n", buf);
	printf("number of frames: %" PRIu32 "\n", slp->num_frames);

	offset += sizeof *slp;
	const struct slp_frame_info *frame;
	size_t frame_size = slp->num_frames * sizeof *frame;
	if (size - offset < frame_size)
		return -1;
	for (uint32_t i = 0, n = slp->num_frames; i < n; ++i) {
		printf("frame %" PRId32 "\n", i);
		slp_frame_info_dump(get_frame(data, i));
	}

	return 0;
}

int main(int argc, char **argv)
{
	FILE *f = NULL;
	int retval = 1;
	struct stat st;
	void *data = NULL;

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "slp");
		goto fail;
	}
	f = fopen(argv[1], "rb");
	if (!f || stat(argv[1], &st)) {
		perror(argv[1]);
		goto fail;
	}
	data = malloc(st.st_size);
	if (!data || fread(data, st.st_size, 1, f) != 1) {
		perror(argv[1]);
		goto fail;
	}
	retval = slp_dump(data, st.st_size);
	if (retval) {
		fputs("bad slp file\n", stderr);
		goto fail;
	}

	retval = 0;
fail:
	if (data)
		free(data);
	if (f)
		fclose(f);
	return retval;
}
