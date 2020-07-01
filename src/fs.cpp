/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include "fs.hpp"

#include "os_macros.hpp"

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

namespace fs {

std::string drs_path(const std::string &root, const std::string &fname) {
	return root + dir_sep + "data" + dir_sep + fname;
}

Blob::Blob(Blob &&b)
	: name(b.name), fd(b.fd)
#if windows
	, fm(b.fm)
#else
	, st(b.st)
#endif
	, data(b.data), size(b.size), map(b.map) {}

#if windows
Blob::Blob(const std::string &name, iofd fd, bool map) : name(name), fd(fd), fm(fd_invalid), map(map) {
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

iofd (open)(const std::string &name) {
	iofd fd;

	if ((fd = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
		return NULL;
	else
		return fd;
}

int (close)(iofd fd) {
	return CloseHandle(fd) == false;
}
#else
int (open)(const std::string &name) {
	return ::open(name.c_str(), O_RDONLY);
}

int (close)(iofd fd) {
	return ::close(fd);
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

}

}
