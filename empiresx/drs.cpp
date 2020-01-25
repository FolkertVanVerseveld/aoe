#include "drs.hpp"

#include <cstdio>
#include <cstdint>
#include <cstring>

#include <stdexcept>

#include <SDL2/SDL_surface.h>

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

DRS::DRS(const std::string &name, iofd fd, bool map) : blob(name, fd, map), hdr((IO_DrsHdr*)blob.get()) {
	// verify header
	if (!hdr->good())
		throw std::runtime_error(std::string("Bad DRS file \"") + name + "\": invalid header");
}

bool DRS::open_item(IO_DrsItem &item, res_id id, uint32_t type) {
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

Palette DRS::open_pal(res_id id) {
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

Background DRS::open_bkg(res_id id) {
	IO_DrsItem item;

	if (!open_item(item, id, DrsType::binary))
		throw std::runtime_error(std::string("Could not load background ") + std::to_string(id));

	Background bkg = { 0 };

	char bkg_name1[16], bkg_name2[16], bkg_name3[16];
	int bkg_id[3];
	char pal_name[16];
	unsigned pal_id;
	char cur_name[16];
	unsigned cur_id;
	char btn_file[16];
	int btn_id;

	char dlg_name[16];
	unsigned dlg_id;
	unsigned bkg_pos, bkg_col;
	unsigned bevel_col[6];
	unsigned text_col[6];
	unsigned focus_col[6];
	unsigned state_col[6];

	const char *data = (char*)hdr + item.offset;
	unsigned shade;

	if (sscanf(data,
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
			bkg_name1, &bkg_id[0],
			bkg_name2, &bkg_id[1],
			bkg_name3, &bkg_id[2],
			pal_name, &pal_id,
			cur_name, &cur_id,
			&shade,
			btn_file, &btn_id,
			dlg_name, &dlg_id,
			&bkg_pos, &bkg_col,
			&bevel_col[0], &bevel_col[1], &bevel_col[2],
			&bevel_col[3], &bevel_col[4], &bevel_col[5],
			&text_col[0], &text_col[1], &text_col[2],
			&text_col[3], &text_col[4], &text_col[5],
			&focus_col[0], &focus_col[1], &focus_col[2],
			&focus_col[3], &focus_col[4], &focus_col[5],
			&state_col[0], &state_col[1], &state_col[2],
			&state_col[3], &state_col[4], &state_col[5]) != 6 + 11 + 4 * 6)
		throw std::runtime_error(std::string("Could not load background ") + std::to_string(id) + ": bad data");

	if (bkg_id[1] < 0)
		bkg_id[1] = bkg_id[0];
	if (bkg_id[2] < 0)
		bkg_id[2] = bkg_id[0];

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

	return bkg;
}

}
