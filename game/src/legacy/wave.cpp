#include "legacy.hpp"

#include "../debug.hpp"

#include <cstdint>

#include <vector>
#include <stdexcept>
#include <string>

namespace aoe {

namespace io {

std::vector<uint8_t> DRS::open_wav(DrsId k) {
	ZoneScoped;
	uint32_t id = (uint32_t)k;
	DrsItem key{ id, 0, 0 };
	auto it = items.find(key);
	if (it == items.end())
		throw std::runtime_error(std::string("invalid audio ID: ") + std::to_string(id));

	const DrsItem &item = *it;

	// fetch data
	in.seekg(item.offset);
	std::vector<uint8_t> data(item.size, 0);
	in.read((char*)data.data(), item.size);

	return data;
}

}

}
