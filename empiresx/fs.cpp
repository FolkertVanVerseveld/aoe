#include "fs.hpp"

#include "os_macros.hpp"
#include "string.hpp"
#include "font.hpp"
#include "drs.hpp"
#include "pe.hpp"

#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <memory>

#if linux
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#define FD_INVALID (-1)

#elif windows
#define FD_INVALID 0
#endif

namespace genie {

FS fs;

#if windows
Blob::Blob(const std::string &name, iofd fd, bool map) : name(name), fd(fd), map(map) {
	LARGE_INTEGER size;

	if (!GetFileSizeEx(fd, &size)) {
		CloseHandle(fd);
		throw std::runtime_error(std::string("Could not retrieve filesize for \"") + name + "\": code " + std::to_string(GetLastError()));
	}

	if (map) {
		if (!(fm = CreateFileMapping(fd, NULL, PAGE_READONLY, 0, 0, NULL))) {
			CloseHandle(fd);
			throw std::runtime_error(std::string("Could not map \"") + name + "\": code " + std::to_string(GetLastError()));
		}

		if (!(data = MapViewOfFile(fm, FILE_MAP_READ, 0, 0, 0))) {
			CloseHandle(fm);
			CloseHandle(fd);
			throw std::runtime_error(std::string("Could not read \"") + name + "\": code " + std::to_string(GetLastError()));
		}
	} else {
		DWORD read;

		if (size.HighPart) {
			CloseHandle(fd);
			throw std::runtime_error(std::string("File too big: ") + name);
		}

		if (!(data = malloc(size.LowPart)) || !ReadFile(fd, data, size.LowPart, &read, NULL) || size.LowPart != read) {
			free(data);
			CloseHandle(fd);
			throw std::runtime_error(std::string("Could not read \"") + name + "\": code " + std::to_string(GetLastError()));
		}
	}

	this->size = size.QuadPart;
}

Blob::~Blob() {
	if (map)
		CloseHandle(fm);
	else
		free(data);
	CloseHandle(fd);
}

iofd iofd_open(const std::string &name) {
	iofd fd;

	if ((fd = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		return NULL;
	else
		return fd;
}
#else
int iofd_open(const std::string &name) {
	return ::open(name.c_str(), O_RDONLY);
}

Blob::Blob(const std::string &name, iofd fd, bool map) : name(name), fd(fd), map(map) {
	if (fstat(fd, &st)) {
		::close(fd);
		throw std::runtime_error(std::string("Could not retrieve filesize for \"") + name + "\": " + std::string(strerror(errno)));
	}

	size = st.st_size;

	if (map) {
		if ((data = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0)) == MAP_FAILED) {
			::close(fd);
			throw std::runtime_error(std::string("Could not map \"") + name + "\": " + std::string(strerror(errno)));
		}
	} else {
		ssize_t n;

		if (!(data = malloc(size)) || (n = read(fd, data, size)) < 0 || (size_t)n != size) {
			free(data);
			::close(fd);
			throw std::runtime_error(std::string("Could not read \"") + name + "\": " + std::string(strerror(errno)));
		}
	}
}

Blob::~Blob() {
	if (map)
		munmap(data, size);
	else
		free(data);
	::close(fd);
}
#endif

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

Font FS::open_ttf(const std::string &name, int ptsize) {
	std::string base(name);
	std::string path(path_cdrom + "system/fonts/" + base), orig(path);
	TTF_Font *f;

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

	throw std::runtime_error(std::string("Could not find font: ") + orig);
}

DRS FS::open_drs(const std::string &name, bool map) {
	std::string base(name);
	std::string path(path_cdrom + "game/data/" + base), orig(path);
	iofd fd;

	if ((fd = iofd_open(path)) != FD_INVALID)
		return DRS(path, fd, map);

	tolower(base);
	path = path_cdrom + "game/data/" + base;

	if ((fd = iofd_open(path)) != FD_INVALID)
		return DRS(path, fd, map);

	toupper(base);
	path = path_cdrom + "GAME/DATA/" + base;

	if ((fd = iofd_open(path)) != FD_INVALID)
		return DRS(path, fd, map);

	throw std::runtime_error(std::string("Could not find DRS: ") + orig);
}

PE FS::open_pe(const std::string &name) {
	std::string base(name);
	std::string path(path_cdrom + "game/" + base), orig(path);
	iofd fd;

	if ((fd = iofd_open(path)) != FD_INVALID)
		return PE(path, fd);

	tolower(base);
	path = path_cdrom + "game/" + base;

	if ((fd = iofd_open(path)) != FD_INVALID)
		return PE(path, fd);

	toupper(base);
	path = path_cdrom + "GAME/" + base;

	if ((fd = iofd_open(path)) != FD_INVALID)
		return PE(path, fd);

	throw std::runtime_error(std::string("Could not find dll: ") + orig);
}

Mix_Music *FS::open_msc(const std::string &name) {
	std::string base(name);
	std::string path(path_cdrom + "game/sound/" + base);
	Mix_Music *msc;

	if ((msc = Mix_LoadMUS(path.c_str())) != 0)
		return msc;

	toupper(base);
	path = path_cdrom + "GAME/SOUND/" + base;

	return Mix_LoadMUS(path.c_str());
}

Mix_Chunk *FS::open_wav(const std::string &name) {
	std::string base(name);
	std::string path(path_cdrom + "game/sound/" + base);
	Mix_Chunk *sfx;

	if ((sfx = Mix_LoadWAV(path.c_str())) != 0)
		return sfx;

	toupper(base);
	path = path_cdrom + "GAME/SOUND/" + base;

	return Mix_LoadWAV(path.c_str());
}

}
