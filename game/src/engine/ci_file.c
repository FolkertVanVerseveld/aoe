#include <ci_file.h>

#include <limits.h>
#include <errno.h>

#if _WIN32
#include <io.h>

#define open _open
#define close _close
#define read _read

typedef ptrdiff_t ssize_t; // this should just work (tm)

#elif __unix__
#include <fcntl.h>
#endif

void ci_file_init(struct ci_file *f)
{
	f->fd = -1;
	f->flags = 0;
	f->mode = 0;
	mblk_init(&f->path, MT_ALLOC, NULL, 0, 0);
}

void ci_file_free(struct ci_file *f)
{
	if (f->fd != -1)
		close(f->fd);

	mblk_free(&f->path);
}

int ci_open(struct ci_file *f, const char *path, unsigned options, int flags, mode_t mode)
{
	int fd, ret = CI_OK;

	if (options & CI_INIT)
		ci_file_init(f);

	if ((fd = open(path, flags, mode)) != -1) {
		// simple case, we're done
		f->fd = fd;
		f->flags = flags;
		f->mode = mode;

		if (options & CI_ALLOC_PATH)
			mstr_init0(&f->path, path);
		else
			mstr_const_set0(&f->path, path);

		return CI_OK;
	}

#if _WIN32
	// _open is already case insensitive, no need to continue
	return CI_DIR;
#else
	if (options & CI_AUTO_CASE) {
		bool upper = false, lower = false;

		for (const char *ptr = path; *ptr; ++ptr) {
			if (!lower && islower((unsigned char)*ptr))
				lower = true;

			if (!upper && isupper((unsigned char)*ptr))
				upper = true;

			if (lower && upper)
				break;
		}

		if (lower && upper) {
			close(fd);
			return CI_EXACT_PATH;
		}
	}

	// TODO stub
	return CI_DIR;
#endif
}

int ci_read(struct ci_file *f, void *ptr, int *size)
{
	int to_read, fd = f->fd;

	if (fd == -1)
		return CI_NO_FILE;

	if (size == NULL)
		return CI_BAD_SIZE_PTR;

	if ((to_read = *size) < 0)
		return CI_READ_TOO_BIG;

	int in = (int)read(fd, ptr, to_read);
	if (in == -1)
		return errno ? CI_ERRNO : CI_READ;

	*size = in;
	return CI_OK;
}

int ci_seek(struct ci_file *f, off_t *ptr, int whence)
{
	int fd = f->fd;

	if (fd == -1)
		return CI_NO_FILE;

	if (ptr == NULL)
		return CI_BAD_SEEK_PTR;

	off_t offset = *ptr, ret;

	ret = lseek(fd, offset, whence);
	if (ret == (off_t)-1)
		return errno ? CI_ERRNO : CI_SEEK;

	*ptr = ret;
	return CI_OK;
}