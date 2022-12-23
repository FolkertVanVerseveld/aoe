#include "../legacy.hpp"

#include <cassert>

#include <stdexcept>
#include <vector>

#include "endian.h"

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
	DrsHdr hdr{ 0 };
	// TODO keep open, move to class vars
	in.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	in.read((char*)&hdr, sizeof(hdr));

	hdr.nlist = le16toh(hdr.nlist);
	hdr.listend = le16toh(hdr.listend);

	printf("%40s %16s\nnlist=%u, listend=%u\n", hdr.copyright, hdr.version, hdr.nlist, hdr.listend);

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

	printf("items read: %llu\n", (unsigned long long)items.size());
}

DrsBkg DRS::open_bkg(DrsId k) {
	uint32_t id = (uint32_t)k;
	DrsItem key{ id, 0, 0 };
	auto it = items.find(key);
	if (it == items.end())
		throw std::runtime_error(std::string("invalid background ID: ") + std::to_string(id));

	DrsBkg bkg{ 0 };
	const DrsItem &item = *it;

	// fetch data
	in.seekg(item.offset);
	std::string data(item.size, ' ');
	in.read(data.data(), item.size);

	// TODO process data
	unsigned shade = 0;

	if (sscanf(data.data(),
			"background1_files %15s none %d -1\n"
			"background2_files %15s none %d -1\n"
			"background3_files %15s none %d -1\n"
			"palette_file %15s %u\n"
			"cursor_file %15s %u\n"
			"shade_amount percent %u\n"
			"button_file %s %d\n"
			"popup_dialog_sin %15s %u\n"
			"background_position %u\n"
			"background_color %u\n"
			"bevel_colors %u %u %u %u %u %u\n"
			"text_color1 %u %u %u\n"
			"text_color2 %u %u %u\n"
			"focus_color1 %u %u %u\n"
			"focus_color2 %u %u %u\n"
			"state_color1 %u %u %u\n"
			"state_color2 %u %u %u\n",
			bkg.bkg_name1, &bkg.bkg_id[0],
			bkg.bkg_name2, &bkg.bkg_id[1],
			bkg.bkg_name3, &bkg.bkg_id[2],
			bkg.pal_name, &bkg.pal_id,
			bkg.cur_name, &bkg.cur_id,
			&shade,
			bkg.btn_file, &bkg.btn_id,
			bkg.dlg_name, &bkg.dlg_id,
			&bkg.bkg_pos, &bkg.bkg_col,
			&bkg.bevel_col[0], &bkg.bevel_col[1], &bkg.bevel_col[2],
			&bkg.bevel_col[3], &bkg.bevel_col[4], &bkg.bevel_col[5],
			&bkg.text_col[0], &bkg.text_col[1], &bkg.text_col[2],
			&bkg.text_col[3], &bkg.text_col[4], &bkg.text_col[5],
			&bkg.focus_col[0], &bkg.focus_col[1], &bkg.focus_col[2],
			&bkg.focus_col[3], &bkg.focus_col[4], &bkg.focus_col[5],
			&bkg.state_col[0], &bkg.state_col[1], &bkg.state_col[2],
			&bkg.state_col[3], &bkg.state_col[4], &bkg.state_col[5]) != 6 + 11 + 4 * 6)
		throw std::runtime_error(std::string("Could not load background ") + std::to_string(id) + ": bad data");

	if (bkg.bkg_id[1] < 0)
		bkg.bkg_id[1] = bkg.bkg_id[0];
	if (bkg.bkg_id[2] < 0)
		bkg.bkg_id[2] = bkg.bkg_id[0];
#if 0
	bkg.bmp[0] = bkg_id[0];
	bkg.bmp[1] = bkg_id[1];
	bkg.bmp[2] = bkg_id[2];

	bkg.pal = pal_id;
	bkg.cursor = cur_id;
	bkg.shade = shade;
	bkg.btn = btn_id;
	bkg.popup = dlg_id;
	bkg.pos = bkg_pos;
	bkg.col = bkg_col;

	for (unsigned i = 0; i < 6; ++i) {
		bkg.bevel[i] = bevel_col[i];
		bkg.text[i] = text_col[i];
		bkg.focus[i] = focus_col[i];
		bkg.state[i] = state_col[i];
	}
#endif

	return bkg;
}

}

}
