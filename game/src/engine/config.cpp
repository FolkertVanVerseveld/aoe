#include "../engine.hpp"

#include <cstdint>

#include <fstream>
#include <string>

#include <tracy/Tracy.hpp>

namespace aoe {

static void write(std::ofstream &out, SDL_Rect r) {
	static_assert(sizeof(int) == sizeof(int32_t));
	out.write((const char*)&r.x, sizeof r.x);
	out.write((const char*)&r.y, sizeof r.y);
	out.write((const char*)&r.w, sizeof r.w);
	out.write((const char*)&r.h, sizeof r.h);
}

Config::Config() : Config("") {}
Config::Config(const std::string &s) : bnds{ 0, 0, 1, 1 }, display{ 0, 0, 1, 1 }, vp{ 0, 0, 1, 1 }, path(s) {}

Config::~Config() {
	if (path.empty())
		return;

	try {
		save(path);
	} catch (std::exception&) {}
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
}

}
