#include <ci_file.h>

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

#if _WIN32
#define open _open
#endif

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

	// TODO stub
	return CI_DIR;
}