/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma warning (disable: 4996)

#include "drs.hpp"

#include <stdexcept>
#include <string>

#include <cstring>
#include <inttypes.h>

namespace genie {

std::string game_dir;

DrsList::DrsList(const DRS &ref, unsigned pos) : drs(ref), pos(pos) {
	const io::DrsList *list = (io::DrsList*)&drs.hdr[1];
	type = list[pos].type;
	offset = list[pos].offset;
	size = list[pos].size;
}

const io::DrsItem &DrsList::operator[](unsigned pos) const noexcept {
	const io::DrsList *list = (io::DrsList*)&drs.hdr[1];
	return ((const io::DrsItem*)((const char*)drs.hdr + list[this->pos].offset))[pos];
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
const uint32_t drs_types[] = {
	drs_type_bin, // binary: dialogs, color palettes
	drs_type_shp, // shp: shape
	drs_type_slp, // slp: list of shapes
	drs_type_wav, // wave: sfx
};

bool DRS::open_item(DrsItem &item, res_id id, DrsType drs_type) const noexcept {
	uint32_t type = drs_types[(unsigned)drs_type];
	const io::DrsList *list = (io::DrsList*)&hdr[1];

	for (unsigned i = 0; i < hdr->nlist; ++i) {
		const io::DrsItem *entry = (io::DrsItem*)((const char*)hdr + list[i].offset);

		if (drs_type != DrsType::max && list[i].type != type)
			continue;

		for (unsigned j = 0; j < list[i].size; ++j)
			if (entry[j].id == id) {
				item.data = (void*)((const char*)hdr + entry[j].offset);
				item.size = entry[j].size;
				return true;
			}
	}

	return false;
}

void io::Slp::reset(void *data, size_t size) {
	if (size < sizeof(io::SlpHdr))
		throw std::runtime_error("Slp data too small");

	hdr = (io::SlpHdr*)data;
	info = (io::SlpFrameInfo*)&hdr[1];
	this->size = size;
}

Image::Image() : id(0), idx(0), surf(nullptr, &SDL_FreeSurface), hotx(0), hoty(0) {}

Animation::Animation(res_id id, const DRS &drs) : slp(drs.open_slp(id)), size(0), images(), id(id), image_count(0), dynamic(false) {
	if (memcmp(slp.hdr->version, "2.0N", 4))
		throw std::runtime_error("Could not load animation: bad header");

	images.reset(new Image[image_count = slp.hdr->frame_count]);

	// stub
}

bool operator<(const Animation &lhs, const Animation &rhs) { return lhs.id < rhs.id; }
bool operator<(const Image &lhs, const Image &rhs) { return lhs.id < rhs.id; }

Dialog::Dialog(res_id id, DialogSettings &&cfg, const DRS &drs)
	: id(id), drs(drs), cfg(cfg), pal(drs.open_pal(this->cfg.pal), &SDL_FreePalette) {}

Dialog DRS::open_dlg(res_id id) const {
	DrsItem item;

	if (!open_item(item, id, DrsType::binary))
		throw std::runtime_error(std::string("Could not load dialog ") + std::to_string(id));

	DialogSettings bkg{0};

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

	const char *data = (char*)item.data;
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
		throw std::runtime_error(std::string("Could not load dialog ") + std::to_string(id) + ": bad data");

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

	return Dialog(id, std::move(bkg), *this);
}

io::Slp DRS::open_slp(res_id id) const {
	DrsItem item;

	if (!open_item(item, id, DrsType::slp))
		throw std::runtime_error(std::string("Could not load shape list ") + std::to_string(id));

	return io::Slp(item.data, item.size);
}

SDL_Palette *DRS::open_pal(res_id id) const {
	DrsItem item;

	if (!open_item(item, id, DrsType::binary))
		throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id));

	const char *ptr = (const char*)item.data;
	unsigned count; int offset;

	if (strncmp(ptr, JASC_PAL, strlen(JASC_PAL)) || sscanf(ptr + strlen(JASC_PAL), "%u\r\n%n", &count, &offset) != 1)
		throw std::runtime_error(std::string("Bad palette header ") + std::to_string(id));

	if (!count)
		return NULL;

	ptr += strlen(JASC_PAL) + offset;
	SDL_Palette *p;

	if (!(p = SDL_AllocPalette(count)))
		throw std::runtime_error("Out of memory");

	unsigned rgb[3];

	for (unsigned i = 0; i < count; ++i, ptr += offset) {
		if (i == count - 1 && sscanf(ptr, "%u %u %u", &rgb[0], &rgb[1], &rgb[2]) != 3)
			throw std::runtime_error(std::string("Bad palette ") + std::to_string(id) + ": bad color index " + std::to_string(i));
		else if (sscanf(ptr, "%u %u %u\r\n%n", &rgb[0], &rgb[1], &rgb[2], &offset) != 3)
			throw std::runtime_error(std::string("Bad palette ") + std::to_string(id) + ": bad color index " + std::to_string(i));

		if (i == 256 - 16) {
			assert("whoah");
		}

		p->colors[i].r = rgb[0];
		p->colors[i].g = rgb[1];
		p->colors[i].b = rgb[2];
		p->colors[i].a = 255;
	}

	return p;
}

}
