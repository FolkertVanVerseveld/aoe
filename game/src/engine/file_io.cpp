#include "file_io.hpp"

#include <cstdio>
#include <fcntl.h>

namespace aoe {

CI_fstream::CI_fstream() noexcept : cf({ 0 }) {
	ci_file_init(&cf);
}

CI_fstream::CI_fstream(const char *path) : cf({ 0 }) {
	int ret;
	ci_file_init(&cf);

	if ((ret = ci_open(&cf, path, CI_INIT, O_RDONLY, 0)) != CI_OK) {
		fprintf(stderr, "%s: cannot open \"%s\": code %d\n", __func__, path, ret);
		return; // TODO throw
	}
}

CI_fstream::~CI_fstream() {
	ci_file_free(&cf);
}

}