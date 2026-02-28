#ifndef AOE_ENGINE_FILE_IO_HPP
#define AOE_ENGINE_FILE_IO_HPP 1

/*
Address some common gotcha's while dealing with file I/O.
*/

#include <ci_file.h>

#include <stdexcept>
#include <string>

namespace aoe {

class CI_fstream final {
public:
	ci_file cf;

	CI_fstream() noexcept;
	CI_fstream(const char *path);
	~CI_fstream();

	int try_read(void *ptr, int &size) noexcept;

	template<typename T> void read(T *ptr, int len) {
		size_t objsize = sizeof *ptr;
		if (len < 0 || objsize > INT_MAX || ((int)objsize > INT_MAX / len))
			throw std::runtime_error("CI_fstream: read: len overflow");

		int size = len * (int)objsize, in = size;
		int ret = try_read((void*)ptr, in);
		if (ret != CI_OK || in != size)
			throw std::runtime_error(std::string("CI_fstream: read failed: code ") + std::to_string(ret));
	}

	int try_seek(off_t to, int whence) noexcept;

	void seek(off_t to, int whence) {
		int ret = try_seek(to, whence);
		if (ret != CI_OK)
			throw std::runtime_error(std::string("CI_fstream: seek failed: code ") + std::to_string(ret));
	}
};

}

#endif