#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace aoe {

namespace io {

class Scenario final {
public:
	std::vector<uint8_t> data;

	// see doc/reverse_engineering/scn for more info
	char version[4];
	uint32_t length;
	uint32_t dword8; // reserved. should be 2
	uint32_t modtime;
	std::string instructions;
	uint32_t players;

	void load(const char *path);
};

}

class ScenarioEditor final {
public:
	unsigned top_btn_idx;
};

}