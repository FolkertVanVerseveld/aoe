/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../empires/cpn.h"

void cpn_init(struct cpn *c)
{
	c->hdr = NULL;
}

void cpn_free(struct cpn *c)
{
	#if DEBUG
	cpn_init(c);
	#endif
}

int cpn_read(struct cpn *dst, const void *data, size_t size)
{
	if (size < sizeof(struct cpn_hdr))
		return CPN_ERR_BAD_HDR;

	dst->hdr = (void*)data;
	// TODO add more bounds checking
	dst->scenarios = (struct cpn_scn*)&dst->hdr[1];

	return 0;
}

void cpn_scn_dump(const struct cpn_scn *s)
{
	char buf[256];

	strncpy(buf, s->description, sizeof buf);
	buf[(sizeof buf) - 1] = '\0';

	printf("scn name: %s\n", buf);

	strncpy(buf, s->filename, sizeof buf);
	buf[(sizeof buf) - 1] = '\0';

	printf("filename: %s\n", buf);
}

void cpn_dump(const struct cpn *c)
{
	char buf[256];

	buf[0] = '\0';
	strncpy(buf, c->hdr->version, 4);
	buf[4] = '\0';
	printf("version: %s\n", buf);
	printf("scenario count: %" PRId32 "\n", c->hdr->scenario_count);

	for (uint32_t i = 0; i < c->hdr->scenario_count; ++i) {
		const struct cpn_scn *s = &c->scenarios[i];
		cpn_scn_dump(s);
	}
}

int main(int argc, char **argv)
{
	FILE *f = NULL;
	void *data = NULL;
	size_t size = 0;
	struct stat st;
	struct cpn c;

	cpn_init(&c);

	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "scn");
		return 1;
	}
	if (!(f = fopen(argv[1], "rb")) || stat(argv[1], &st) || !(data = malloc(st.st_size)) || fread(data, st.st_size, 1, f) != 1) {
		perror(argv[1]);
		return 1;
	}
	fclose(f);

	if (cpn_read(&c, data, st.st_size)) {
		fprintf(stderr, "%s: bad scenario\n", argv[1]);
		return 1;
	}
	cpn_dump(&c);

	cpn_free(&c);
	free(data);

	return 0;
}
