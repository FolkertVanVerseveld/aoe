#pragma once

/**
 * Legacy windows Portable Executable crap
 *
 * NOTE this code does not work on big-endian machines!
 */

#include <cstdint>

#include "fs.hpp"
#include "drs.hpp"

namespace genie {

/* You should never need anything in this namespace */
namespace io {

struct mz final {
	uint16_t e_magic;
	// ...
};

struct dos final {
	// ...
};

struct pehdr final {
	// ...
};

struct coffopthdr final {
	// ...
};

struct peopthdr32 final {
	// ...
};

struct peopthdr64 final {
	// ...
};

union peopthdr final {
	struct peopthdr32 hdr32;
	struct peophdr64 hdr64;
};

struct peditem final {
	// ...
};

struct peddef final {
	// ...
};

union peddir final {
	struct peditem items[16];
	struct peddef data;
};

}

/**
 * Wrapper to load Portable Executable resources.
 * This wrapper is always preferred over any OS API for PE resources because it
 * does not care if the PE arch and OS arch match (i.e. we can load resources
 * on a 64 bit arch even though the DLL is 32 bit and vice versa).
 */
class PE {
	Blob blob;
public:
	PE(const std::string &name, iofd fd, bool map);

	int load_res(unsigned type, res_id id, void **data, size_t &count);
	int load_string(std::string &s, res_id id);
};

}