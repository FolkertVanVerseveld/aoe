#include "fs.hpp"

#include "os_macros.hpp"
#include "string.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <memory>

namespace genie {

FS fs;

bool FS::find_cdrom(OS &os) {
#if windows
	std::string path("D:\\SYSTEM\\FONTS\\ARIAL.TTF");

	for (char dl = 'D'; dl <= 'Z'; ++dl) {
		path[0] = dl;
		if (std::ifstream(path, std::ios::binary)) {
			path_cdrom = "D:\\";
			path_cdrom[0] = dl;
			return true;
		}
	}
#else
	path_cdrom = "/media/cdrom/";
	if (std::ifstream("/media/cdrom/SYSTEM/FONTS/ARIAL.TTF", std::ios::binary))
		return true;
	if (std::ifstream("/media/cdrom/system/fonts/arial.ttf", std::ios::binary))
		return true;

	path_cdrom = "/media/" + os.username + "/cdrom/";
	if (std::ifstream(path_cdrom + "SYSTEM/FONTS/ARIAL.TTF", std::ios::binary))
		return true;
	if (std::ifstream(path_cdrom + "system/fonts/arial.ttf", std::ios::binary))
		return true;
#endif
	return false;
}

void FS::init(OS &os) {
	if (!find_cdrom(os))
		throw std::runtime_error("Could not find CD-ROM");
}

Font FS::open_ttf(const std::string& name, int ptsize) {
	std::string base(name);
	std::string path(path_cdrom + "system/fonts/" + base);
	TTF_Font* f;

	if ((f = TTF_OpenFont(path.c_str(), ptsize)) != NULL)
		return Font(f, ptsize);

	tolower(base);
	path = path_cdrom + "system/fonts/" + base;

	if ((f = TTF_OpenFont(path.c_str(), ptsize)) != NULL)
		return Font(f, ptsize);

	toupper(base);
	path = path_cdrom + "SYSTEM/FONTS/" + base;

	if ((f = TTF_OpenFont(path.c_str(), ptsize)) != NULL)
		return Font(f, ptsize);

	throw std::runtime_error(std::string("Could not find font: ") + path);
}

}
