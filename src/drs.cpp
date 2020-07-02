/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "drs.hpp"

#include <stdexcept>
#include <string>

#include <cstring>

namespace genie {

DrsList::DrsList(const DRS &ref, unsigned pos) : drs(ref), pos(pos) {
	const io::DrsList *list = (io::DrsList*)&drs.hdr[1];
	type = list[pos].type;
	offset = list[pos].offset;
	size = list[pos].size;
}

const DrsItem DrsList::operator[](unsigned pos) const noexcept {
	const io::DrsList *list = (io::DrsList*)&drs.hdr[1];
	const io::DrsItem &entry = ((io::DrsItem*)((const char*)drs.hdr + list[this->pos].offset))[pos];

	return DrsItem(entry.id, type, entry.offset, (const char*)drs.hdr + entry.offset, entry.size);
}

DRS::DRS(const std::string &name, iofd fd, bool map) : blob(name, fd, map), hdr((io::DrsHdr*)blob.get()) {
	// verify header
	if (!hdr->good())
		throw std::runtime_error(std::string("Bad DRS file \"") + name + "\": invalid header");
}

bool io::DrsHdr::good() const {
	return strncmp(version, "1.00tribe", strlen("1.00tribe")) == 0;
}

// DRS file formats in little endian byte order
static const uint32_t drs_types[] = {
	0x62696e61, // binary: dialogs, color palettes
	0x73687020, // shp: shape
	0x736c7020, // slp: list of shapes
	0x77617620, // wave: sfx
};

bool DRS::open_item(io::DrsItem &item, res_id id, DrsType drs_type) const {
	uint32_t type = drs_types[(unsigned)drs_type];
	const io::DrsList *list = (io::DrsList*)&hdr[1];

	for (unsigned i = 0; i < hdr->nlist; ++i) {
		const io::DrsItem *entry = (io::DrsItem*)((const char*)hdr + list[i].offset);

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

}
