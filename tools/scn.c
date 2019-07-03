/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "../empires/serialization.h"
#include "../empires/scn.h"

void scn_init(struct scn *s)
{
	s->hdr = NULL;
	s->data = NULL;
}

void scn_free(struct scn *s)
{
	#if DEBUG
	scn_init(s);
	#endif
}

int scn_read(struct scn *dst, const void *map, size_t size)
{
	if (size < sizeof(struct scn_hdr))
		return SCN_ERR_BAD_HDR;

	dst->hdr = (void*)map;
	// TODO add more bounds checking
	ptrdiff_t offdata = sizeof(struct scn_hdr) + dst->hdr->instructions_length + 8;
	dst->data = (unsigned char*)map + offdata;
	dst->size = size;

	return 0;
}

void scn_dump(const struct scn *s)
{
	char buf[256];

	buf[0] = '\0';
	strncpy(buf, s->hdr->version, 4);
	buf[4] = '\0';
	printf("version: %s\n", buf);
	if (s->hdr->instructions_length)
		printf("scenario instructions: %s\n", s->hdr->instructions);
	time_t now = s->hdr->time;
	printf("saved at: %s", asctime(gmtime(&now)));
	ptrdiff_t offdata = (const unsigned char*)s->data - (const unsigned char*)s->hdr;
	printf("compressed data at: %zX\n", (size_t)offdata);

	void *data;
	size_t data_size;
	if (raw_inflate(&data, &data_size, s->data, s->size - offdata) == 0) {
		FILE *f;

		if (!(f = fopen("scn.out", "wb"))) {
			perror("scn.out");
			return;
		}
		puts("dump");
		fwrite(data, data_size, 1, f);
		free(data);
		fclose(f);
	} else {
		fputs("bad compressed data\n", stderr);
	}
}

int main(int argc, char **argv)
{
	FILE *f = NULL;
	void *data = NULL;
	struct stat st;
	struct scn s;

	scn_init(&s);

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "scn");
		return 1;
	}
	if (!(f = fopen(argv[1], "rb")) || stat(argv[1], &st) || !(data = malloc(st.st_size)) || fread(data, st.st_size, 1, f) != 1) {
		perror(argv[1]);
		return 1;
	}
	fclose(f);

	if (scn_read(&s, data, st.st_size)) {
		fprintf(stderr, "%s: bad scenario\n", argv[1]);
		return 1;
	}
	scn_dump(&s);

	scn_free(&s);
	free(data);

	return 0;
}
