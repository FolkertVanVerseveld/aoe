#include "scenario.hpp"

#include <cassert>
#include <fstream>

#include "../engine/endian.h"

namespace aoe {

namespace io {

void Scenario::load(const char *path) {
	std::ifstream in(path, std::ios_base::binary);

	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	in.seekg(0, std::ios_base::end);
	long long end = in.tellg();
	in.seekg(0);
	long long beg = in.tellg();

	assert(end >= beg);
	size_t size = (unsigned long long)(end - beg);

	data.clear();
	data.resize(size);
	in.read((char*)data.data(), size);

	in.seekg(0);
	in.read((char*)&version, sizeof(version));
	in.read((char*)&length, sizeof(length));
	in.read((char*)&dword8, sizeof(dword8));
	in.read((char*)&modtime, sizeof(modtime));

	uint32_t isize;
	in.read((char*)&isize, sizeof(isize));

	length = le32toh(length);
	dword8 = le32toh(dword8);
	isize = le32toh(isize);

	instructions.clear();
	instructions.resize(isize);
	in.read(instructions.data(), isize);

	uint32_t padding;
	in.read((char*)&padding, sizeof(padding));

	in.read((char*)&players, sizeof(players));

	players = le32toh(players);

	// TODO read compressed data
}

}

}
