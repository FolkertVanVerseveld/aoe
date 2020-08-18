#pragma once

#include <cstddef>
#include <cstdint>

#include <string>
#include <vector>
#include <utility>

namespace genie {

namespace io {

struct ScnHdr2;

struct ScnHdr final {
	char version[4];
	uint32_t header_size, dword8;
	uint32_t time;
	uint32_t instructions_length;
	char instructions[];

	constexpr size_t size() const noexcept {
		return offsetof(struct ScnHdr, header_size) + header_size;
	}

	bool constexpr good(size_t fsize) const noexcept {
		return fsize >= offsetof(struct ScnHdr, header_size) + 4 && fsize >= size();
	}

	ScnHdr2 *next() noexcept {
		return (ScnHdr2*)((char*)this + header_size);
	}
};

struct ScnHdr2 final {
	uint32_t dword14;
	uint32_t player_count;

	uint8_t *next() noexcept {
		return (uint8_t*)&this[1];
	}
};

class Scenario final {
public:
	std::vector<uint8_t> data;

	io::ScnHdr *hdr;
	io::ScnHdr2 *hdr2;

	std::string description;
	std::vector<std::pair<size_t, size_t>> sections;

	int32_t map_width, map_height;

	std::vector<uint8_t> tiles;
	std::vector<int8_t> hmap;
	std::vector<uint8_t> overlay;

	Scenario(std::vector<uint8_t> &raw);
};

}

}
