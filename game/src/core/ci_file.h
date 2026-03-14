#ifndef CI_FILE_H
#define CI_FILE_H 1

#include "mblk.h"

#include <sys/stat.h> // fstat, mode_t, windows: _off_t

#if _WIN32
typedef int mode_t; // _PMode for _open
typedef _off_t off_t;
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

// ci_open return status codes. TODO this is mixed with MB_*
#define CI_OK 0
// failed to iterate directory or dirname not found
#define CI_DIR 1
// auto case is set and path does not match.
// NOTE: this error is only returned on unix
#define CI_EXACT_PATH 2
// ci_file must be opened before it can read or write or ci_file was closed
#define CI_NO_FILE 3
// `size_t *size` in ci_read is NULL
#define CI_BAD_SIZE_PTR 4
// `off_t *offset` in ci_seek is NULL
#define CI_BAD_SEEK_PTR 5
// windows: size > UNSIGNED_MAX
#define CI_READ_TOO_BIG 6
// ci_read failed for unknown reason
#define CI_READ 7
// ci_seek failed for unknown reason
#define CI_SEEK 8
// ci_read/ci_write failed and errno was set
#define CI_ERRNO 9
// out of memory
#define CI_NOMEM 10

void ci_file_init(struct ci_file *f);
//void ci_file_close(struct ci_file *f);
void ci_file_free(struct ci_file *f);

/**
 * Case Insensitive OPEN. Like open(2), but path may be case insensitive.
 * Does not support O_CREAT yet. When \a mode is not needed in open(2)
 * (e.g. flags == O_RDONLY), value is ignored.
 */
int ci_open(struct ci_file *f, const char *path, unsigned options, int flags, mode_t mode);

/**
 * Read data into \a ptr of \a size bytes. Number of read bytes will be stored
 * in \a size. On error, ptr and size are unspecified.
 * NOTE: \a size is an int as linux and windows don't support
 *       `size > INT_MAX - XXX` anyways.
 */
int ci_read(struct ci_file *f, void *ptr, int *size);

int ci_seek(struct ci_file *f, off_t *offset, int whence);

#if __cplusplus
}
#endif

#endif
