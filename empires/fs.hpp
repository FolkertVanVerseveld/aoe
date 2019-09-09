/* Copyright 2019-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#pragma once

#include <genie/fs.h>

// simple wrapper for fs_blob
struct FileBlob {
	struct fs_blob blob;

	FileBlob(const char *name, unsigned mode=FS_MODE_READ | FS_MODE_MAP) {
		fs_blob_init(&blob);
		int err;
		if ((err = fs_blob_open(&blob, name, mode)))
			throw err;
	}

	~FileBlob() {
		fs_blob_close(&blob);
	}
};
