#include "legacy.hpp"

#include "../debug.hpp"

#include <stdexcept>
#include <string>

namespace aoe {

namespace io {

static const char JASC_PAL[] = {
	"JASC-PAL\r\n"
	"0100\r\n"
	"256\r\n"
};

std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> DRS::open_pal(DrsId k) {
	ZoneScoped;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> p(nullptr, SDL_FreePalette);

	uint32_t id = (uint32_t)k;
	DrsItem key{ id, 0, 0 };
	auto it = items.find(key);
	if (it == items.end())
		throw std::runtime_error(std::string("invalid palette ID: ") + std::to_string(id));

	const DrsItem &item = *it;

	// fetch data
	in.seekg(item.offset);
	std::string data(item.size, ' ');
	in.read(data.data(), item.size);

	if (item.size < strlen(JASC_PAL))
		throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id) + ": bad header");

	const char *ptr = data.data() + strlen(JASC_PAL);

	// NOTE we implicitly assume data is '\0' terminated, which it is by default as c++ strings are '\0' terminated.

	unsigned lines = 256;
	p.reset(SDL_AllocPalette(lines));

	for (unsigned i = 0; i < lines; ++i) {
		unsigned rgb[3];
		const char *nl;

		if (!(nl = strchr(ptr, '\n')) || sscanf(ptr, "%u %u %u", &rgb[0], &rgb[1], &rgb[2]) != 3)
			throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id) + ": bad data");

		p->colors[i].r = rgb[0];
		p->colors[i].g = rgb[1];
		p->colors[i].b = rgb[2];
		p->colors[i].a = SDL_ALPHA_OPAQUE;

		ptr += nl - ptr + 1;
	}

	return p;
}

}

}
