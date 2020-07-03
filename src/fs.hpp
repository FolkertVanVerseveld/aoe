/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#pragma once

// fix stupid posix incompatibilities
#define _CRT_DECLARE_NONSTDC_NAMES 1

#include "os_macros.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#if windows
#include <Windows.h>
#include <io.h>
#endif

#include <string>

namespace genie {

#if windows
static constexpr char dir_sep = '\\';
static constexpr HANDLE fd_invalid = NULL;
typedef HANDLE iofd;
#else
static constexpr char dir_sep = '/';
static constexpr int fd_invalid = -1;
typedef int iofd;
#endif

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
	Blob(const Blob&) = delete;
	~Blob();

	const void *get() const { return data; }
	size_t length() const { return size; }
};

std::string drs_path(const std::string &root, const std::string &fname);
std::string msc_path(const std::string &root, const std::string &fname);

iofd (open)(const std::string&);
int (close)(iofd);

}
