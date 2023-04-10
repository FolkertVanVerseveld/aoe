#include "../legacy.hpp"

#include "../debug.hpp"
#include "../engine/endian.h"

#include <stdexcept>
#include <string>

namespace aoe {

namespace io {

/** Array of resource items. */
struct IO_DrsList final {
	uint32_t type;   /**< Data Resource format. */
	uint32_t offset; /**< Raw offset in archive. */
	uint32_t size;   /**< Size in bytes. */
};

struct DrsHdr final {
	char copyright[40];
	char version[16];
	uint32_t nlist;
	uint32_t listend;
};

DRS::DRS(const std::string &path) : in(path, std::ios_base::binary), items() {
	ZoneScoped;
	in.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	DrsHdr hdr{ 0 };
	in.read((char*)&hdr, sizeof(hdr));

	hdr.nlist = le32toh(hdr.nlist);
	hdr.listend = le32toh(hdr.listend);

	//printf("%40s %16s\nnlist=%u, listend=%u\n", hdr.copyright, hdr.version, hdr.nlist, hdr.listend);

	// populate list of items

	// read lists
	std::vector<IO_DrsList> lists(hdr.nlist, { 0 });
	in.read((char*)lists.data(), hdr.nlist * sizeof(IO_DrsList));

	// parse lists
	for (IO_DrsList &l : lists) {
		l.type = le32toh(l.type);
		l.offset = le32toh(l.offset);
		l.size = le32toh(l.size);

		// find list start
		in.seekg(l.offset, std::ios_base::beg);

		for (uint_fast32_t i = 0; i < l.size; ++i) {
			DrsItem item{ 0 };
			in.read((char*)&item, sizeof(item));

			item.id = le32toh(item.id);
			item.offset = le32toh(item.offset);
			item.size = le32toh(item.size);

			auto ins = items.emplace(item);

			if (!ins.second)
				throw std::runtime_error(std::string("duplicate ID: ") + std::to_string(item.id));
		}
	}

	//printf("items read: %llu\n", (unsigned long long)items.size());
}

}

}
