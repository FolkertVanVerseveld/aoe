#pragma once

#include "os.hpp"
#include "font.hpp"

#include <string>

namespace genie {

class FS;

extern FS fs;

class FS final {
private:
	std::string path_cdrom;
	bool find_cdrom(OS& os);

public:
	void init(OS &os);

	/**
	 * Open specified font and try 3 times to get filename right: try the original given name first, then lowercase and finally uppercase
	 */
	Font open_ttf(const std::string &name, int ptsize);
};

}