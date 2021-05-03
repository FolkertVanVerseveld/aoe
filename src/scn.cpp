#include "scn.hpp"

#include <cstdio>
#include <cassert>
#include <inttypes.h>

#include <cstring>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

static constexpr uint32_t section_separator = 0xffffff9d;

extern std::string raw_inflate(const void *src, size_t size);

uint8_t *next_section(std::vector<uint8_t> &data, size_t &off) {
	std::array<uint8_t, 4> a{0x9d, 0xff, 0xff, 0xff};
	auto search = std::search(data.begin() + off, data.end(), a.begin(), a.end());
	if (search == data.end())
		return nullptr;

	off = search - data.begin();
	return &*search;
}

namespace genie {

namespace io {

std::map<uint8_t, std::pair<uint8_t, uint8_t>> tconv({
	{0, {2, 0}},
	{1, {3, 0}},
	{6, {1, 0}},
	{22, {3, 1}},
});

Scenario::Scenario(std::vector<uint8_t> &raw) : data(), hdr(nullptr), hdr2(nullptr), description(), sections(0), map_width(0), map_height(0), tiles(), subimg(), hmap(), overlay()
{
	hdr = (ScnHdr*)raw.data();

	if (!hdr->good(raw.size()))
		throw std::runtime_error("Bad scenario header");

	hdr2 = hdr->next();
	description.resize(hdr->instructions_length);
	memcpy(description.data(), hdr->instructions, hdr->instructions_length);

	std::string payload(raw_inflate(hdr2->next(), raw.size() - hdr->size() - sizeof(*hdr2)));

	size_t start = hdr->size() + sizeof(*hdr2);

	data.resize(hdr->size() + sizeof(*hdr2) + payload.size());
	memcpy(data.data(), raw.data(), hdr->size() + sizeof(*hdr2));
	memcpy(data.data() + start, payload.data(), payload.size());

	// raw is not guaranteed to be kept alive, so update pointers
	hdr = (ScnHdr*)data.data();
	hdr2 = hdr->next();

	size_t off = start;
	uint8_t *ptr;

	while ((ptr = next_section(data, off)) != nullptr) {
		sections.emplace_back(start, off - start);
		off += 4;
		start = off;
	}

	sections.emplace_back(off, data.size() - off);

	// through empirical testing, 5 sections are mandatory
	if (sections.size() != 5)
		throw std::runtime_error(std::string("Invalid section count: expected 5, got ") + std::to_string(sections.size()));

	assert(sections[4].first < data.size());
	int32_t *dw = (int32_t*)((char*)data.data() + sections[4].first);
	map_width = dw[0];
	map_height = dw[1];

	assert(map_width && map_height);

	size_t size = (size_t)map_width * map_height;

	tiles.reserve(size);
	subimg.reserve(size);
	hmap.reserve(size);
	overlay.reserve(size);

	// first byte seems type, second height and third something like overlay?
	uint8_t *db = (uint8_t*)&dw[2];

	for (unsigned x = 0; x < map_width; ++x) {
		for (unsigned y = 0; y < map_height; ++y) {
			tiles.push_back(db[3 * (y * map_width + x) + 0]);
			subimg.push_back(0);
			hmap.push_back(db[3 * (y * map_width + x) + 1]);
			overlay.push_back(db[3 * (y * map_width + x) + 2]);
		}
	}

	for (size_t i = 0; i < size; ++i) {
		auto search = tconv.find(tiles[i]);

		if (search == tconv.end())
			continue;

		tiles[i] = search->second.first;
		subimg[i] = search->second.second;
	}

#if 0
	// TODO fix corners and overlays
	std::vector<uint8_t> copy_tiles(tiles);

	for (unsigned y = 1; y < map_height - 1; ++y) {
		for (unsigned x = 1; x < map_width - 1; ++x) {
			// check for conflicts
			size_t pos[9];
			uint8_t id[9];
			// 0 1 2
			// 3 4 5
			// 6 7 8
			pos[0] = (y + 1) * map_width + (x - 1);
			pos[1] = (y + 1) * map_width + x;
			pos[2] = (y + 1) * map_width + (x + 1);
			pos[3] = y * map_width + (x - 1);
			pos[4] = y * map_width + x;
			pos[5] = y * map_width + (x + 1);
			pos[6] = (y - 1) * map_width + (x - 1);
			pos[7] = (y - 1) * map_width + x;
			pos[8] = (y - 1) * map_width + (x + 1);

			// fetch tiles
			id[0] = copy_tiles[pos[0]]; id[1] = copy_tiles[pos[1]]; id[2] = copy_tiles[pos[2]];
			id[3] = copy_tiles[pos[3]]; id[4] = copy_tiles[pos[4]]; id[5] = copy_tiles[pos[5]];
			id[6] = copy_tiles[pos[6]]; id[7] = copy_tiles[pos[7]]; id[8] = copy_tiles[pos[8]];

			switch (id[4]) {
				case 2:
					// do easy cases first
					//   1
					// 3 4 5
					//   7
					// corners with multiple water pieces
					if (id[1] == 2 && id[3] == 51 && id[5] == 2 && id[7] == 51)
						id[4] = 91;
					else if (id[1] == 2 && id[3] == 2 && id[5] == 51 && id[7] == 51)
						id[4] = 93;
					// straight
					else if (id[3] == 2 && id[5] == 2 && id[7] == 51)
						id[4] = 101;
					// corners with single water piece
					else if (id[5] == 2 && id[7] == 2 && id[8] == 51)
						id[4] = 97;
					else if (id[3] == 2 && id[6] == 51 && id[7] == 2)
						id[4] = 95;
					break;
			}

			tiles[pos[4]] = id[4];
		}
	}
#endif
}

}

}
