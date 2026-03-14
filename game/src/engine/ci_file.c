#include <ci_file.h>

#include <limits.h>
#include <errno.h>
#include <ctype.h>

#if _WIN32
#include <io.h>

#define open _open
#define close _close
#define read _read

typedef ptrdiff_t ssize_t; // this should just work (tm)

#elif __unix__
#include <dirent.h>   // fdopendir, closedir
#include <strings.h>  // strcasecmp
#include <sys/stat.h> // fstat
#include <fcntl.h>
#endif

static inline bool str_is_mixed_case(const char *s)
{
	bool upper = false, lower = false;

	for (; *s; ++s) {
		if (!lower && islower((unsigned char)*s))
			lower = true;

		if (!upper && isupper((unsigned char)*s))
			upper = true;

		if (lower && upper)
			break;
	}

	return lower && upper;
}

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
	if ((options & CI_AUTO_CASE) && str_is_mixed_case(path)) {
		close(fd);
		return CI_EXACT_PATH;
	}

	struct mblk ipath, pathrem;
	DIR *d = NULL;
	struct dirent *dp;
	mstr_init0(&pathrem, NULL);

	// XXX skip redundant initial part of paths?
	// no need... it's non-trivial and this function is too complex already

	if (mstr_init0(&ipath, path)) {
		ret = CI_NOMEM;
		goto fail;
	}

	// recurse backwards trying to find case insensitive subpath that works
	for (char *ptr; (ptr = mstr_rchr(&ipath, '/')) != NULL;) {
		if ((ret = mstr_append_rev(&pathrem, ptr)) != MB_OK ||
			(ret = mstr_shrink_ptr(&ipath, ptr)) != MB_OK)
		{
			goto fail;
		}

		if ((fd = open(ipath.buf.str, O_RDONLY)) != -1)
			// path up to this points seems valid
			break;
	}

	if (fd == -1) {
		fd = open(*path == '/' ? "/" : "./", O_RDONLY);
		if (fd == -1)
			goto fail;
	}

	// now try to work forwards again
	mstr_bpop(&pathrem);

	for (char *ptr; (ptr = mstr_rchr(&pathrem, '/')) != NULL;) {
		if ((ret = mstr_addch(&ipath, '/')) != MB_OK)
			goto fail;

		// don't use pointer here as mblk manipulation may relocate buf
		size_t ilen = ipath.len;

		if ((ret = mstr_append_rev(&ipath, ptr + 1)) != MB_OK ||
			(ret = mstr_shrink_ptr(&pathrem, ptr)) != MB_OK)
		{
			goto fail;
		}

		// directory name we are going to look for
		const char *basename = &ipath.buf.cstr[ilen];

		// duplicate dir separators result in empty dirname, skip!
		if (!*basename) {
			mstr_bpop(&ipath); // strip
			continue;
		}

		if (d)
			closedir(d);

		if (!(d = fdopendir(fd))) {
			ret = CI_DIR;
			goto fail;
		}

		int fd2 = -1;

		while ((dp = readdir(d)) != NULL) {
			if (strcasecmp(basename, dp->d_name))
				continue;

			if ((ret = mstr_shrink_ptr(&ipath, basename)) != MB_OK ||
				(ret = mstr_addstr(&ipath, dp->d_name)) != MB_OK)
			{
				goto fail;
			}

			fd2 = openat(fd, dp->d_name, O_RDONLY);
			break;
		}

		if (fd2 != -1) {
			// strictly not necessary due to closedir, but just in case
			close(fd);
			fd = fd2;
		} else {
			goto fail;
		}
	}

	if (d)
		closedir(d);

	if (!(d = fdopendir(fd))) {
		ret = CI_DIR;
		goto fail;
	}

	size_t ilen = ipath.len + 1;

	if ((ret = mstr_addch(&ipath, '/')) != MB_OK ||
		(ret = mstr_append_rev(&ipath, pathrem.buf.cstr)) != MB_OK)
	{
		goto fail;
	}

	const char *basename = &ipath.buf.cstr[ilen];
	int fd2 = -1;

	while ((dp = readdir(d)) != NULL) {
		if (strcasecmp(basename, dp->d_name))
			continue;

		fd2 = openat(fd, dp->d_name, ipath.buf.cstr);
		if (fd2 == -1)
			goto fail;

		if ((ret = mstr_shrink_ptr(&ipath, basename)) != MB_OK ||
			(ret = mstr_addstr(&ipath, dp->d_name)) != MB_OK)
		{
			goto fail;
		}
		break;
	}

	closedir(d);

	if (fd2 == -1)
		goto fail;

	close(fd);
	fd2 = fd;

	f->fd = fd;
	f->flags = flags;
	f->mode = mode;
	mstr_cpy(&f->path, &ipath);

	mblk_free(&pathrem);

	return CI_OK;
fail:
	if (d)
		closedir(d);

	if (fd != -1)
		close(fd);

	mblk_free(&ipath);
	mblk_free(&pathrem);

	return ret;
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