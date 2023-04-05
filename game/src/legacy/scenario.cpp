#include "scenario.hpp"

#include <cstddef>
#include <cassert>
#include <fstream>

#include "../engine/endian.h"

// yes, including .c's is evil, but... we have no choice :/
#include <miniz.c>

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

	// read compressed data

	// 8 + length is where compressed scenario data starts
	unsigned long long data_start = 8 + length;

	in.seekg(data_start);

	// make a very rough lazy estimate how much the uncompressed data will be
	// yeah we could try to increase it step-wise when it fails, but it's legacy anyways so unless it breaks often, i won't bother
	size_t max_inflated_size = 64 * 1024 * 1024;

	std::vector<uint8_t> inflated(max_inflated_size);

	unsigned long inflated_size = inflated.size();

	mz_stream stream{ 0 };

	stream.next_in = data.data() + data_start;
	stream.avail_in = data.size() - data_start;

	stream.next_out = inflated.data();
	stream.avail_out = inflated.size();

	int status;

	if ((status = mz_inflateInit2(&stream, -MZ_DEFAULT_WINDOW_BITS))) {
		fprintf(stderr, "mz v%s failed: %s\n", mz_version(), mz_error(status));
		return;
	}

	status = mz_inflate(&stream, MZ_FINISH);
	mz_inflateEnd(&stream);

	if (status != MZ_STREAM_END) {
		fprintf(stderr, "inflate error: %s\n", mz_error(status));
		return;
	}

	inflated.resize(stream.total_out);

	data = std::move(inflated);
}

}

}
