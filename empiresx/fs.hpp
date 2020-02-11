#pragma once

#include "os_macros.hpp"

#include "os.hpp"

#include <cstddef>
#include <string>

#if windows
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <SDL2/SDL_mixer.h>

namespace genie {

#if windows
typedef HANDLE iofd;
#else
typedef int iofd;
#endif

class FS;
class Font;
class DRS;
class PE;

extern FS fs;

/** Disk-based raw game asset controller. */
class Blob final {
	std::string name;
	iofd fd;
#if windows
	HANDLE fm;
#else
	struct stat st;
#endif
	void *data;
	size_t size;
public:
	const bool map;

	Blob(const std::string &name, iofd fd, bool map);
	~Blob();

	const void *get() const { return data; }
	size_t length() const { return size; }
};

/** Generic unique global disk I/O controller */
class FS final {
private:
	std::string path_cdrom;
	bool find_cdrom(OS &os);

public:
	void init(OS &os);

	/**
	 * Open specified font and try 3 times to get filename right: try the original given name first, then lowercase and finally uppercase
	 */
	Font open_ttf(const std::string &name, int ptsize);
	/** Open data resources set using same naming technique as above. */
	DRS open_drs(const std::string &name, bool map=true);

	PE open_pe(const std::string &name);

	Mix_Music *open_msc(const std::string &name);
};

}
