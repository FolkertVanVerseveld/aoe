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

	if ((ret = ci_open(&cf, path, CI_INIT, O_RDONLY | O_BINARY, 0)) != CI_OK) {
		fprintf(stderr, "%s: cannot open \"%s\": code %d\n", __func__, path, ret);
		return; // TODO throw
	}
}

CI_fstream::~CI_fstream() {
	ci_file_free(&cf);
}

int CI_fstream::try_open(const char *path) noexcept {
	ci_file_free(&cf);
	return ci_open(&cf, path, CI_INIT, O_RDONLY | O_BINARY, 0);
}

int CI_fstream::try_read(void *ptr, int &size) noexcept {
	return ci_read(&cf, ptr, &size);
}

int CI_fstream::try_seek(off_t to, int whence) noexcept {
	return ci_seek(&cf, &to, whence);
}

}