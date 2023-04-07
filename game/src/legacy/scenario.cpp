#include "scenario.hpp"

#include <cstddef>
#include <cassert>
#include <fstream>

#include "../engine/endian.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "../debug.hpp"

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

	// header must start with '1.' there are more versions than we support, but this should be sufficient.
	if (version[0] != '1' || version[1] != '.')
		throw BadScenarioError("not a scn or cpx file");

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

void ScenarioEditor::create_map() {
	map_width = std::clamp(map_gen_width, 0, UINT16_MAX + 1);
	map_height = std::clamp(map_gen_height, 0, UINT16_MAX + 1);
}

void ScenarioEditor::load(const std::string &path) {
	ZoneScoped;
	using json = nlohmann::json;

	std::ifstream in(path);

	// let c++ take care of any errors
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try {
		json data(json::parse(in));

		unsigned version = data["version"];
		if (version != ScenarioEditor::version)
			throw io::BadScenarioError("version unsupported");

		map_width = data["tiles"]["w"];
		map_height = data["tiles"]["h"];
	} catch (std::exception &e) {
		throw std::runtime_error(e.what());
	}
}

void ScenarioEditor::save(const std::string &path) {
	ZoneScoped;
	using json = nlohmann::json;
	json data;

	data["version"] = ScenarioEditor::version;
	data["tiles"]["w"] = map_width;
	data["tiles"]["h"] = map_height;

	// TODO add tiles data

	std::ofstream out(path);// , std::ios_base::binary | std::ios_base::trunc);

	// let c++ take care of any errors
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	out << data.dump(4) << std::endl;
}

}
