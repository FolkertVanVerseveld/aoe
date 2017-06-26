/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
quick and dirty program that exactly replicates a file by generating a
dissassembly that can be fed to nasm to produce the exact same file.
*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {
	int fd = -1, ret = 1;
	FILE *out = NULL;
	ssize_t zd;
	#define BUFSZ 4096
	char buf[BUFSZ], dbuf[BUFSZ];
	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argc > 0 ? argv[0] : "convert");
		return 1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror(argv[1]);
		goto fail;
	}
	out = fopen("a.out", "wb");
	if (!out) {
		perror("a.out");
		goto fail;
	}
	while ((zd = read(fd, buf, sizeof buf)) > 0) {
		unsigned i, n;
		for (n = (unsigned)zd, i = 0; i < n; ++i) {
			snprintf(dbuf, sizeof dbuf, "db 0x%02hhX\n", buf[i]);
			if (fwrite(dbuf, 1, 8, out) != 8) {
				perror("fwrite");
				goto fail;
			}
		}
	}
	ret = 0;
fail:
	if (out) fclose(out);
	if (fd != -1) close(fd);
	return ret;
}
