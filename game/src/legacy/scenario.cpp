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

	if (inflated.size() < 0x1000) {
		fprintf(stderr, "corrupt inflated data: too small: at least 0x1000, got: %llu\n", (unsigned long long)inflated.size());
		return;
	}

	data = std::move(inflated);

	// parse data
	size_t pos = 4;
	uint32_t magic = u32(pos);

	const uint32_t magic_exp = 0x3f933333;

	if (magic != magic_exp) {
		fprintf(stderr, "corrupt header: expected %X, got %X\n", magic_exp, magic);
		return;
	}

	// parse name aliases
	std::vector<std::string> player_aliases;

	for (unsigned i = 0; i <= 0xf; ++i) {
		pos = 0x08 + 0x100 * i;
		player_aliases.emplace_back(str(pos, 256));
	}

	// parse section 1
	pos = 0x1000;

	for (unsigned nr = 0; nr <= 0x10; ++nr) {
		uint32_t idx = u32(pos);
		uint32_t dword4 = u32(pos), dword8 = u32(pos), dwordC = u32(pos);

		// TODO stub
	}

	uint8_t some_byte = u8(pos), byte_exp = 0xbf;

	if (some_byte != byte_exp) {
		fprintf(stderr, "some byte mismatch: expected %X, got %X\n", byte_exp, some_byte);
		return;
	}

	std::string filename(str16(pos)), instructions(str16(pos));

	std::vector<std::string> some_strings;

	for (unsigned i = 0; i < 9 + 5; ++i)
		some_strings.emplace_back(str16(pos));

	some_strings.clear();

	uint16_t some_word = u16(pos), word_exp = 1;
	if (some_word != word_exp) {
		fprintf(stderr, "some word mismatch: expected %X, got %X\n", word_exp, some_word);
		return;
	}

	for (unsigned i = 0; i < 48; ++i)
		some_strings.emplace_back(str16(pos));

	for (unsigned p = 0; p < 16; ++p) {
		uint32_t some_ai_length = u32(pos), some_entities_length = u32(pos), some_length = u32(pos);

		std::string some_ai;
		for (size_t i = 0; i < some_ai_length; ++i)
			some_ai.push_back(u8(pos));

		if (!some_ai.empty())
			printf("some_ai:\n%s\n", some_ai.c_str());

		std::string some_entities;
		for (size_t i = 0; i < some_entities_length; ++i)
			some_entities.push_back(u8(pos));

		if (!some_entities.empty())
			printf("some entities:\n%s\n", some_entities.c_str());

		std::string some_string;
		for (size_t i = 0; i < some_length; ++i)
			some_string.push_back(u8(pos));

		if (!some_string.empty())
			printf("some string:\n%s\n", some_string.c_str());
	}

	const uint32_t magic_sep = 0xffffff9d;
	uint32_t some_dword = u32(pos);

	if (some_dword != magic_sep) {
		fprintf(stderr, "bad section separator: expected %X, got %X\n", magic_sep, some_dword);
		return;
	}

	std::vector<uint32_t> some_dwords;

	for (uint32_t v; (v = u32(pos)) != magic_sep;)
		some_dwords.emplace_back(v);

	printf("TODO pos: %llX\n", (unsigned long long)pos);

#if 0
	for (uint32_t sep; (sep = u32(pos)) != magic_sep;) {
		pos -= 4; // revert bogus separator
		some_strings.emplace_back(str16(pos));
	}

	// TODO parse remaining sections and data
	some_byte = u8(pos);
#endif
}

uint8_t Scenario::u8(size_t &pos) const {
	uint8_t v = data.at(pos);
	++pos;
	return v;
}

uint16_t Scenario::u16(size_t &pos) const {
	uint16_t v = data.at(pos) | (data.at(pos + 1) << 8);
	pos += 2;
	return v;
}

uint32_t Scenario::u32(size_t &pos) const {
	uint32_t v = data.at(pos) | (data.at(pos + 1) << 8) | (data.at(pos + 2) << 16) | (data.at(pos + 3) << 24);
	pos += 4;
	return v;
}

std::string Scenario::str16(size_t &pos) const {
	uint16_t length = u16(pos);
	std::string s;

	for (unsigned i = 0; i < length; ++i)
		s.push_back(u8(pos));

	return s;
}

std::string Scenario::str(size_t &pos, size_t max) const {
	std::string s;

	for (size_t i = 0; i < max; ++i) {
		uint8_t v = u8(pos);

		if (!v)
			break;

		s.push_back(v);
	}

	return s;
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
