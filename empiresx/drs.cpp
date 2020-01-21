#include "drs.hpp"

#include <cstdio>
#include <cstdint>
#include <cstring>

#include <stdexcept>

namespace genie {

/** Array of resource items. */
struct IO_DrsList final {
	uint32_t type;   /**< Data Resource format. */
	uint32_t offset; /**< Raw offset in archive. */
	uint32_t size;   /**< Size in bytes. */
};

bool IO_DrsHdr::good() const {
	return strncmp(version, "1.00tribe", strlen("1.00tribe")) == 0;
}

DRS::DRS(const std::string& name, iofd fd, bool map) : blob(name, fd, map), hdr((IO_DrsHdr*)blob.get()) {
	// verify header
	if (!hdr->good())
		throw std::runtime_error(std::string("Bad DRS file \"") + name + "\": invalid header");
}

bool DRS::open_item(IO_DrsItem &item, uint32_t id, uint32_t type) {
	const IO_DrsList *list = (IO_DrsList*)&hdr[1];

	for (unsigned i = 0; i < hdr->nlist; ++i) {
		const IO_DrsItem *entry = (IO_DrsItem*)((char*)hdr + list[i].offset);

		if (list[i].type != type)
			continue;

		for (unsigned j = 0; j < list[i].size; ++j)
			if (entry[j].id == id) {
				item = entry[j];
				return true;
			}
	}

	return false;
}

const char JASC_PAL[] = {
	"JASC-PAL\r\n"
	"0100\r\n"
	"256\r\n"
};

Palette DRS::open_pal(uint32_t id) {
	IO_DrsItem item;

	if (!open_item(item, id, DrsType::binary))
		throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id));

	Palette pal = { 0 };

	const char *ptr = (char*)hdr + item.offset;

	if (strncmp(ptr, JASC_PAL, strlen(JASC_PAL)))
		throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id) + ": bad header");

	ptr += strlen(JASC_PAL);

	for (unsigned i = 0; i < 256; ++i) {
		unsigned rgb[3];
		const char *nl;

		if (!(nl = strchr(ptr, '\n')) || sscanf(ptr, "%u %u %u", &rgb[0], &rgb[1], &rgb[2]) != 3)
			throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id) + ": bad data");

		pal.tbl[i].r = rgb[0];
		pal.tbl[i].g = rgb[1];
		pal.tbl[i].b = rgb[2];
		pal.tbl[i].a = 255;

		ptr += nl - ptr + 1;
	}

	return pal;
}

}
