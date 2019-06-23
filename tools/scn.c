/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

	dst->hdr = map;
	// TODO add more bounds checking
	dst->data = &dst->hdr[1];

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
	printf("compressed data at: %zX\n", (size_t)sizeof(struct scn_hdr) + s->hdr->instructions_length + 8);
}

int main(int argc, char **argv)
{
	FILE *f = NULL;
	void *data = NULL;
	size_t size = 0;
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
