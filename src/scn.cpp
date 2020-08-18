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

Scenario::Scenario(std::vector<uint8_t> &raw) : data(), hdr(nullptr), hdr2(nullptr), description(), sections(0), map_width(0), map_height(0), tiles(), hmap(), overlay() {
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
	hmap.reserve(size);
	overlay.reserve(size);

	// first byte seems type, second height and third something like overlay?
	uint8_t *db = (uint8_t*)&dw[2];

	for (unsigned x = 0; x < map_width; ++x) {
		for (unsigned y = 0; y < map_height; ++y) {
			tiles.push_back(db[3 * (y * map_width + x) + 0]);
			hmap.push_back(db[3 * (y * map_width + x) + 1]);
			overlay.push_back(db[3 * (y * map_width + x) + 2]);
		}
	}

	// TODO convert tiles data
	// XXX or use same id numbering internally...

	for (size_t i = 0; i < size; ++i) {
		switch (tiles[i]) {
			case 0:
				tiles[i] = 26;
				break;
			case 1:
				tiles[i] = 51;
				break;
			default:
				continue;
		}
	}
}

}

}
