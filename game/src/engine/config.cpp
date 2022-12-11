#include "../engine.hpp"

#include <cstdint>

#include <fstream>
#include <string>

#include <tracy/Tracy.hpp>

namespace aoe {

static void read(std::ifstream &in, SDL_Rect &r) {
	static_assert(sizeof(int) == sizeof(uint32_t));
	in.read((char*)&r.x, sizeof(r.x));
	in.read((char*)&r.y, sizeof(r.y));
	in.read((char*)&r.w, sizeof(r.w));
	in.read((char*)&r.h, sizeof(r.h));
}

static void read(std::ifstream &in, std::string &s, uint32_t max=UINT32_MAX) {
	uint32_t n;
	in.read((char*)&n, sizeof(n));

	if (n > max)
		throw std::runtime_error("string overflow");

	s.resize(n, ' ');
	in.read(s.data(), n);
}

static void write(std::ofstream &out, SDL_Rect r) {
	static_assert(sizeof(int) == sizeof(int32_t));
	out.write((const char*)&r.x, sizeof r.x);
	out.write((const char*)&r.y, sizeof r.y);
	out.write((const char*)&r.w, sizeof r.w);
	out.write((const char*)&r.h, sizeof r.h);
}

static void write(std::ofstream &out, const std::string &s) {
	if (s.size() > UINT32_MAX)
		throw std::runtime_error("string overflow");

	uint32_t n = s.size();
	out.write((const char*)&n, sizeof(n));
	out.write(s.data(), n);
}

Config::Config(Engine &e) : Config(e, "") {}
Config::Config(Engine &e, const std::string &s) : e(e), bnds{ 0, 0, 1, 1 }, display{ 0, 0, 1, 1 }, vp{ 0, 0, 1, 1 }, path(s) {}

Config::~Config() {
	if (path.empty())
		return;

	try {
		save(path);
	} catch (std::exception&) {}
}

void Config::load(const std::string &path) {
	ZoneScoped;
	std::ifstream in(path, std::ios_base::binary);

	// let c++ take care of any errors
	in.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	uint32_t magic = 0x06ce09f6, actual_magic;

	in.read((char*)&actual_magic, sizeof(actual_magic));

	if (magic != actual_magic)
		throw std::runtime_error("magic mismatch");

	read(in, bnds);
	read(in, display);
	read(in, vp);

	Audio &sfx = e.sfx;
	uint32_t dw;

	in.read((char*)&dw, sizeof(dw));

	for (uint32_t i = 0; i < dw; ++i) {
		uint32_t id;
		std::string path;

		in.read((char*)&id, sizeof(id));
		read(in, path, UINT16_MAX);

		sfx.jukebox[(MusicId)id] = path;
	}
}

void Config::save(const std::string &path) {
	ZoneScoped;
	std::ofstream out(path, std::ios_base::binary | std::ios_base::trunc);

	// let c++ take care of any errors
	out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	uint32_t magic = 0x06ce09f6;

	out.write((const char*)&magic, sizeof magic);
	write(out, bnds);
	write(out, display);
	write(out, vp);

	Audio &sfx = e.sfx;
	uint32_t dw = sfx.jukebox.size();

	out.write((const char*)&dw, sizeof(dw));

	for (auto kv : sfx.jukebox) {
		dw = (unsigned)kv.first;
		out.write((const char*)&dw, sizeof(dw));
		write(out, kv.second);
	}
}

}
