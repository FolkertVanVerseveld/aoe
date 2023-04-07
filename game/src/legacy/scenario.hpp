#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include <stdexcept>

namespace aoe {

namespace io {

class BadScenarioError final : public std::runtime_error {
public:
	explicit BadScenarioError(const std::string &s) : std::runtime_error(s.c_str()) {}
	explicit BadScenarioError(const char *msg) : std::runtime_error(msg) {}
};

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
	int map_gen_type;
	int map_gen_width, map_gen_height;
	int map_gen_terrain_type;

	int map_width, map_height;

	static constexpr unsigned version = 1;

	void create_map();

	void load(const std::string &path);
	void save(const std::string &path);
};

}