#ifndef AOE_ENGINE_FILE_IO_HPP
#define AOE_ENGINE_FILE_IO_HPP 1

/*
Address some common gotcha's while dealing with file I/O.
*/

#include <ci_file.h>

namespace aoe {

class CI_fstream final {
public:
	ci_file cf;

	CI_fstream() noexcept;
	CI_fstream(const char *path);
	~CI_fstream();
};

}

#endif