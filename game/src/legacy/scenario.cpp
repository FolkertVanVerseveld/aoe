#include "scenario.hpp"

#include <cstddef>
#include <cassert>
#include <fstream>

#include "../engine/endian.h"

#include <algorithm>

#include <nlohmann/json.hpp>

#include "../debug.hpp"
#include "../game.hpp"
#include "../world/terrain.hpp"

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
		fprintf(stderr, "corrupt inflated data: too small: at least 4096, got: %llu\n", (unsigned long long)inflated.size());
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

	for (unsigned i = 0; i < 48; ++i) // 3 * 16 player data??
		some_strings.emplace_back(str16(pos));

	player_data.clear();

	for (unsigned p = 0; p < 16; ++p) {
		uint32_t some_ai_length = u32(pos), some_entities_length = u32(pos), some_length = u32(pos);

		std::string ai;
		for (size_t i = 0; i < some_ai_length; ++i)
			ai.push_back(u8(pos));

		std::string entities;
		for (size_t i = 0; i < some_entities_length; ++i)
			entities.push_back(u8(pos));

		std::string some_string;
		for (size_t i = 0; i < some_length; ++i)
			some_string.push_back(u8(pos));

		player_data.emplace_back(ai, entities, some_string);
	}

	const uint32_t magic_sep = 0xffffff9d;
	uint32_t some_dword = u32(pos);

	if (some_dword != magic_sep) {
		fprintf(stderr, "bad section separator: expected %X, got %X\n", magic_sep, some_dword);
		return;
	}

	std::vector<uint32_t> some_dwords;

	next_section(some_dwords, pos);
	// dwords 1 0 0 0 0 0 0 0 900 9000 some 3s

	some_dwords.clear();

	for (unsigned i = 0; i < 10; ++i)
		some_dwords.emplace_back(u32(pos));

	next_section(some_dwords, pos);
	next_section(some_dwords, pos);

	some_dwords.clear();

	printf("terrain pos: %llX\n", (unsigned long long)pos);

	// TODO parse terrain data
	uint32_t w = u32(pos), h = u32(pos);

	if (!w || !h) {
		fprintf(stderr, "invalid map size: %ux%u\n", w, h);
		return;
	}

	if (w > SIZE_MAX / h || (uint64_t)w * h > UINT32_MAX) {
		fprintf(stderr, "map too big: %ux%u\n", w, h);
		fprintf(stderr, "max size: %llu\n", std::min<unsigned long long>(SIZE_MAX, UINT32_MAX));
		return;
	}

	size_t count = (size_t)w * h;

	this->w = w;
	this->h = h;

	tile_types.clear();
	tile_height.clear();
	tile_meta.clear();
	entities.clear();

	tile_types.resize(count);
	tile_height.resize(count);
	tile_meta.resize(count);

	for (size_t x = 0; x < w; ++x) {
		for (size_t y = 0; y < h; ++y) {
			uint8_t type = u8(pos);

			// TODO convert properly
			switch (type) {
			case 0x16: type = (unsigned)TileType::deepwater; break;
			case 0x01: type = (unsigned)TileType::water; break;
			case 0x02: type = (unsigned)TileType::water_desert; break; // grass water edge
			case 0x00: type = (unsigned)TileType::grass; break;
			case 0x06: type = (unsigned)TileType::desert; break;
			case 0x0d:
				type = (unsigned)TileType::desert;
				entities.emplace(desert_tree(x + 2 * y), (float)x, (float)y, 0);
				break; // desert palm trees on desert
			case 0x0a:
				type = (unsigned)TileType::grass;
				entities.emplace(grass_tree(x + 2 * y), (float)x, (float)y, 0);
				break; // grass trees
			case 0x04: type = (unsigned)TileType::water; break; // shallows
			case 0x14:
				type = (unsigned)TileType::grass;
				entities.emplace(desert_tree(x + 2 * y), (float)x, (float)y, 0);
				break; // desert palm tree on grass
			default: type = (unsigned)TileType::deepwater; break; // unknown
			}

			size_t idx = y * w + x;

			tile_types[idx] = type;
			tile_height[idx] = u8(pos);
			tile_meta[idx] = u8(pos);
		}
	}

	// second pass to fix tiles
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			size_t idx = y * w + x;
			// prefer a slow and dumb algorithm that just works
			TileType type = (TileType)tile_types[idx];
			if (type == TileType::water_desert) {
				auto nn = fhvn(tile_types, w, h, x, y, TileType::desert);
				unsigned edges = 0, subimage = 0;

				// count water tiles
				for (TileType t : nn)
					if (t == TileType::water)
						++edges;

				switch (edges) {
				case 1:
					if (nn[0] == TileType::water)
						subimage = 11;
					else if (nn[1] == TileType::water)
						subimage = 8;
					else if (nn[2] == TileType::water)
						subimage = 9;
					else
						subimage = 10;
					break;
				case 2:
					if (nn[1] == TileType::water && nn[3] == TileType::water)
						subimage = 0;
					else if (nn[1] == TileType::water && nn[0] == TileType::water)
						subimage = 1;
					else if (nn[2] == TileType::water && nn[3] == TileType::water)
						subimage = 2;
					else
						subimage = 3;
					break;
				case 0:
					// find diagonal neighbors
					nn = fdn(tile_types, w, h, x, y, TileType::desert);

					if (nn[0] == TileType::water)
						subimage = 5;
					else if (nn[1] == TileType::water)
						subimage = 7;
					else if (nn[2] == TileType::water)
						subimage = 4;
					else
						subimage = 6;

					break;
				}

				// update
				tile_types[idx] = Terrain::tile_id(type, subimage);
			} else if (type == TileType::deepwater) {
				// deepwater drs id 20006
				auto nn = fhvn(tile_types, w, h, x, y, TileType::deepwater);
				unsigned bits = 0;

				if (nn[1] == TileType::water)
					bits |= 1 << 0;
				if (nn[0] == TileType::water)
					bits |= 1 << 1;
				if (nn[3] == TileType::water)
					bits |= 1 << 2;
				if (nn[2] == TileType::water)
					bits |= 1 << 3;

				if (bits) {
					type = TileType::deepwater_water;
					bits <<= 16 - 4 - 3;
				}

				// update
				tile_types[idx] = Terrain::tile_id(type, bits);
			} else if (type == TileType::grass) {
				auto nn = fhvn(tile_types, w, h, x, y, TileType::grass);
				unsigned bits = 0;

				if (nn[1] == TileType::desert || nn[1] == TileType::water_desert)
					bits |= 1 << 0;
				if (nn[0] == TileType::desert || nn[0] == TileType::water_desert)
					bits |= 1 << 1;
				if (nn[3] == TileType::desert || nn[3] == TileType::water_desert)
					bits |= 1 << 2;
				if (nn[2] == TileType::desert || nn[2] == TileType::water_desert)
					bits |= 1 << 3;

				if (bits) {
					type = TileType::grass_desert;
					bits <<= 16 - 4 - 3;
				}

				// update
				tile_types[idx] = Terrain::tile_id(type, bits);
			}
		}
	}

	// third pass for heightmap data
	for (size_t y = 0; y < h; ++y) {
		for (size_t x = 0; x < w; ++x) {
			size_t idx = y * w + x;

			TileType type = (TileType)tile_types[idx];
			if (type == TileType::water || type == TileType::deepwater)
				continue;

			auto th = tile_height[idx];

			unsigned img = 0;
			auto hh = hhvn(tile_height, w, h, x, y, th);

			if (hh[0] > th) {
				if (hh[1] > th) {
					img = 22;
				} else if (hh[2] > th) {
					img = 23;
				} else {
					img = 16;
				}
			} else if (hh[2] > th) {
				if (hh[3] > th) {
					img = 21;
				} else {
					img = 15;
				}
			} else if (hh[1] > th) {
				if (hh[3] > th) {
					img = 24;
				} else {
					img = 14;
				}
			} else if (hh[3] > th) {
				img = 13;
			}

			if (!img) {
				hh = hdn(tile_height, w, h, x, y, th);

				if (hh[0] > th) {
					img = 10;
				} else if (hh[1] > th) {
					img = 12;
				} else if (hh[2] > th) {
					img = 11;
				} else if (hh[3] > th) {
					img = 9;
				}
			}

			tile_types[idx] |= Terrain::tile_id((TileType)0, img);
		}
	}

	printf("TODO pos: %llX\n", (unsigned long long)pos);
	// TODO parse remaining sections and data
}

void Scenario::next_section(std::vector<uint32_t> &lst, size_t &pos) {
	const uint32_t magic_sep = 0xffffff9d;

	lst.clear();

	for (uint32_t v; (v = u32(pos)) != magic_sep;)
		lst.emplace_back(v);
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

// TODO extract scenarioeditor functions to separate file
void ScenarioEditor::create_map(Game &g) {
	map_width  = std::clamp(map_gen_width , 0, UINT16_MAX + 1);
	map_height = std::clamp(map_gen_height, 0, UINT16_MAX + 1);

	ScenarioSettings scn;
	scn.width = map_width;
	scn.height = map_height;

	g.resize(scn);
	g.terrain_create();
}

void ScenarioEditor::load(const io::Scenario &scn) {
	map_gen_width  = map_width  = scn.w;
	map_gen_height = map_height = scn.h;
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
