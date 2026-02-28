#ifndef CI_FILE_H
#define CI_FILE_H 1

#include "mblk.h"

#if _WIN32
typedef int mode_t; // _PMode for _open
#elif __unix__
#include <sys/stat.h> // fstat, mode_t
#endif

#if __cplusplus
extern "C" {
#endif

/** Case Insensitive FILE */
struct ci_file {
	int fd, flags;
	mode_t mode;
	struct mblk path;
};

#define CI_FILE_INIT {.fd = -1, .flags = 0, .mode = 0, .path = MBLK_INIT}

#define CI_INIT       0x01 // call ci_file_init on passed ci_file
#define CI_ALLOC_PATH 0x02 // always make a copy of specified path to ci_open
#define CI_AUTO_CASE  0x04 // unix only: if path has both upper and lowercase characters, don't match case insensitively

#define CI_OK 0
#define CI_DIR 1
// auto case is set and path does not match.
// NOTE: this error is only returned on unix
#define CI_EXACT_PATH 2

void ci_file_init(struct ci_file *f);
void ci_file_close(struct ci_file *f);
void ci_file_free(struct ci_file *f);

/**
 * Case Insensitive OPEN. Like open(2), but path may be case insensitive.
 * Does not support O_CREAT yet. When \a mode is not needed in open(2)
 * (e.g. flags == O_RDONLY), value is ignored.
 */
int ci_open(struct ci_file *f, const char *path, unsigned options, int flags, mode_t mode);

#if __cplusplus
}
#endif

#endif
