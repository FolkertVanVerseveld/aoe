#include "scn.hpp"

#include <cstring>
#include <algorithm>
#include <array>
#include <stdexcept>

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

Scenario::Scenario(std::vector<uint8_t> &raw) : data(), hdr(nullptr), hdr2(nullptr), description(), sections(0) {
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
}

}

}
