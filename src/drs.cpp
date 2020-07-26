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

Image::Image() : id(0), idx(0), surf(std::nullopt), bnds(), dynamic(false) {}

Image::Image(res_id id, unsigned idx, const DRS &drs, size_t size, unsigned p)
	: Image(id, drs.open_slp(id), idx, size, p) {}

Image::Image(res_id id, const io::Slp &slp, unsigned idx, size_t size, unsigned p)
	: id(id), idx(idx), surf(std::nullopt), dynamic(load(slp, idx, p)) {}

#pragma warning (disable: 26451)

static unsigned cmd_or_next(const unsigned char **cmd, unsigned n)
{
	const unsigned char *ptr = *cmd;
	unsigned v = *ptr >> n;
	if (!v)
		v = *(++ptr);
	*cmd = ptr;
	return v;
}

bool Image::load(const io::Slp &slp, unsigned idx, unsigned player) {
	bool dynamic = false;
	const io::SlpFrameInfo *info = &slp.info[idx];

	// adjust row for gaia
	if (player == io::max_players - 1)
		++player;

	bnds.x = info->hotspot_x;
	bnds.y = info->hotspot_y;

	surf.emplace<std::vector<uint8_t>>(info->width * info->height);

	auto &pixels = std::get<std::vector<uint8_t>>(surf);

	const io::SlpFrameRowEdge *edge = (const io::SlpFrameRowEdge*)((const char*)slp.hdr + info->outline_table_offset);
	const unsigned char *cmd = (const unsigned char*)((const char*)slp.hdr + info->cmd_table_offset + 4 * info->height);

	for (int y = 0, h = info->height; y < h; ++y, ++edge) {
		if (edge->left_space == io::invalid_edge || edge->right_space == io::invalid_edge)
			continue;

		int line_size = info->width - edge->left_space - edge->right_space;

		// fill line with random data so any incorrect data is shown as garbled data
		for (int x = edge->left_space, w = x + line_size, p = info->width; x < w; ++x)
			pixels[y * p + x] = rand();

		for (int i = edge->left_space, x = i, w = x + line_size, p = info->width; i <= w; ++i, ++cmd) {
			unsigned command = *cmd & 0xf, count;

			// TODO figure out if lesser skip behaves different compared to aoe2

			switch (*cmd) {
			case 0x03:
				for (count = *++cmd; count; --count)
					pixels[y * p + x++] = 0;
				continue;
			case 0xF7: pixels[y * p + x++] = cmd[1];
			case 0xE7: pixels[y * p + x++] = cmd[1];
			case 0xD7: pixels[y * p + x++] = cmd[1];
			case 0xC7: pixels[y * p + x++] = cmd[1];
			case 0xB7: pixels[y * p + x++] = cmd[1];
				// dup 10
			case 0xA7: pixels[y * p + x++] = cmd[1];
			case 0x97: pixels[y * p + x++] = cmd[1];
			case 0x87: pixels[y * p + x++] = cmd[1];
			case 0x77: pixels[y * p + x++] = cmd[1];
			case 0x67: pixels[y * p + x++] = cmd[1];
			case 0x57: pixels[y * p + x++] = cmd[1];
			case 0x47: pixels[y * p + x++] = cmd[1];
			case 0x37: pixels[y * p + x++] = cmd[1];
			case 0x27: pixels[y * p + x++] = cmd[1];
			case 0x17: pixels[y * p + x++] = cmd[1];
				++cmd;
				continue;
				// player color fill
			case 0xF6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 15
			case 0xE6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 14
			case 0xD6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 13
			case 0xC6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 12
			case 0xB6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 11
			case 0xA6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 10
			case 0x96: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 9
			case 0x86: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 8
			case 0x76: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 7
			case 0x66: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 6
			case 0x56: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 5
			case 0x46: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 4
			case 0x36: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 3
			case 0x26: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 2
			case 0x16: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 1
				dynamic = true;
				continue;
				// XXX pixel count if lower_nibble == 4: 1 + cmd >> 2
			case 0xfc: pixels[y * p + x++] = *++cmd; // fill 63
			case 0xf8: pixels[y * p + x++] = *++cmd; // fill 62
			case 0xf4: pixels[y * p + x++] = *++cmd; // fill 61
			case 0xf0: pixels[y * p + x++] = *++cmd; // fill 60
			case 0xec: pixels[y * p + x++] = *++cmd; // fill 59
			case 0xe8: pixels[y * p + x++] = *++cmd; // fill 58
			case 0xe4: pixels[y * p + x++] = *++cmd; // fill 57
			case 0xe0: pixels[y * p + x++] = *++cmd; // fill 56
			case 0xdc: pixels[y * p + x++] = *++cmd; // fill 55
			case 0xd8: pixels[y * p + x++] = *++cmd; // fill 54
			case 0xd4: pixels[y * p + x++] = *++cmd; // fill 53
			case 0xd0: pixels[y * p + x++] = *++cmd; // fill 52
			case 0xcc: pixels[y * p + x++] = *++cmd; // fill 51
			case 0xc8: pixels[y * p + x++] = *++cmd; // fill 50
			case 0xc4: pixels[y * p + x++] = *++cmd; // fill 49
			case 0xc0: pixels[y * p + x++] = *++cmd; // fill 48
			case 0xbc: pixels[y * p + x++] = *++cmd; // fill 47
			case 0xb8: pixels[y * p + x++] = *++cmd; // fill 46
			case 0xb4: pixels[y * p + x++] = *++cmd; // fill 45
			case 0xb0: pixels[y * p + x++] = *++cmd; // fill 44
			case 0xac: pixels[y * p + x++] = *++cmd; // fill 43
			case 0xa8: pixels[y * p + x++] = *++cmd; // fill 42
			case 0xa4: pixels[y * p + x++] = *++cmd; // fill 41
			case 0xa0: pixels[y * p + x++] = *++cmd; // fill 40
			case 0x9c: pixels[y * p + x++] = *++cmd; // fill 39
			case 0x98: pixels[y * p + x++] = *++cmd; // fill 38
			case 0x94: pixels[y * p + x++] = *++cmd; // fill 37
			case 0x90: pixels[y * p + x++] = *++cmd; // fill 36
			case 0x8c: pixels[y * p + x++] = *++cmd; // fill 35
			case 0x88: pixels[y * p + x++] = *++cmd; // fill 34
			case 0x84: pixels[y * p + x++] = *++cmd; // fill 33
			case 0x80: pixels[y * p + x++] = *++cmd; // fill 32
			case 0x7c: pixels[y * p + x++] = *++cmd; // fill 31
			case 0x78: pixels[y * p + x++] = *++cmd; // fill 30
			case 0x74: pixels[y * p + x++] = *++cmd; // fill 29
			case 0x70: pixels[y * p + x++] = *++cmd; // fill 28
			case 0x6c: pixels[y * p + x++] = *++cmd; // fill 27
			case 0x68: pixels[y * p + x++] = *++cmd; // fill 26
			case 0x64: pixels[y * p + x++] = *++cmd; // fill 25
			case 0x60: pixels[y * p + x++] = *++cmd; // fill 24
			case 0x5c: pixels[y * p + x++] = *++cmd; // fill 23
			case 0x58: pixels[y * p + x++] = *++cmd; // fill 22
			case 0x54: pixels[y * p + x++] = *++cmd; // fill 21
			case 0x50: pixels[y * p + x++] = *++cmd; // fill 20
			case 0x4c: pixels[y * p + x++] = *++cmd; // fill 19
			case 0x48: pixels[y * p + x++] = *++cmd; // fill 18
			case 0x44: pixels[y * p + x++] = *++cmd; // fill 17
			case 0x40: pixels[y * p + x++] = *++cmd; // fill 16
			case 0x3c: pixels[y * p + x++] = *++cmd; // fill 15
			case 0x38: pixels[y * p + x++] = *++cmd; // fill 14
			case 0x34: pixels[y * p + x++] = *++cmd; // fill 13
			case 0x30: pixels[y * p + x++] = *++cmd; // fill 12
			case 0x2c: pixels[y * p + x++] = *++cmd; // fill 11
			case 0x28: pixels[y * p + x++] = *++cmd; // fill 10
			case 0x24: pixels[y * p + x++] = *++cmd; // fill 9
			case 0x20: pixels[y * p + x++] = *++cmd; // fill 8
			case 0x1c: pixels[y * p + x++] = *++cmd; // fill 7
			case 0x18: pixels[y * p + x++] = *++cmd; // fill 6
			case 0x14: pixels[y * p + x++] = *++cmd; // fill 5
			case 0x10: pixels[y * p + x++] = *++cmd; // fill 4
			case 0x0c: pixels[y * p + x++] = *++cmd; // fill 3
			case 0x08: pixels[y * p + x++] = *++cmd; // fill 2
			case 0x04: pixels[y * p + x++] = *++cmd; // fill 1
				continue;
			case 0xfd: pixels[y * p + x++] = 0; // skip 63
			case 0xf9: pixels[y * p + x++] = 0; // skip 62
			case 0xf5: pixels[y * p + x++] = 0; // skip 61
			case 0xf1: pixels[y * p + x++] = 0; // skip 60
			case 0xed: pixels[y * p + x++] = 0; // skip 59
			case 0xe9: pixels[y * p + x++] = 0; // skip 58
			case 0xe5: pixels[y * p + x++] = 0; // skip 57
			case 0xe1: pixels[y * p + x++] = 0; // skip 56
			case 0xdd: pixels[y * p + x++] = 0; // skip 55
			case 0xd9: pixels[y * p + x++] = 0; // skip 54
			case 0xd5: pixels[y * p + x++] = 0; // skip 53
			case 0xd1: pixels[y * p + x++] = 0; // skip 52
			case 0xcd: pixels[y * p + x++] = 0; // skip 51
			case 0xc9: pixels[y * p + x++] = 0; // skip 50
			case 0xc5: pixels[y * p + x++] = 0; // skip 49
			case 0xc1: pixels[y * p + x++] = 0; // skip 48
			case 0xbd: pixels[y * p + x++] = 0; // skip 47
			case 0xb9: pixels[y * p + x++] = 0; // skip 46
			case 0xb5: pixels[y * p + x++] = 0; // skip 45
			case 0xb1: pixels[y * p + x++] = 0; // skip 44
			case 0xad: pixels[y * p + x++] = 0; // skip 43
			case 0xa9: pixels[y * p + x++] = 0; // skip 42
			case 0xa5: pixels[y * p + x++] = 0; // skip 41
			case 0xa1: pixels[y * p + x++] = 0; // skip 40
			case 0x9d: pixels[y * p + x++] = 0; // skip 39
			case 0x99: pixels[y * p + x++] = 0; // skip 38
			case 0x95: pixels[y * p + x++] = 0; // skip 37
			case 0x91: pixels[y * p + x++] = 0; // skip 36
			case 0x8d: pixels[y * p + x++] = 0; // skip 35
			case 0x89: pixels[y * p + x++] = 0; // skip 34
			case 0x85: pixels[y * p + x++] = 0; // skip 33
			case 0x81: pixels[y * p + x++] = 0; // skip 32
			case 0x7d: pixels[y * p + x++] = 0; // skip 31
			case 0x79: pixels[y * p + x++] = 0; // skip 30
			case 0x75: pixels[y * p + x++] = 0; // skip 29
			case 0x71: pixels[y * p + x++] = 0; // skip 28
			case 0x6d: pixels[y * p + x++] = 0; // skip 27
			case 0x69: pixels[y * p + x++] = 0; // skip 26
			case 0x65: pixels[y * p + x++] = 0; // skip 25
			case 0x61: pixels[y * p + x++] = 0; // skip 24
			case 0x5d: pixels[y * p + x++] = 0; // skip 23
			case 0x59: pixels[y * p + x++] = 0; // skip 22
			case 0x55: pixels[y * p + x++] = 0; // skip 21
			case 0x51: pixels[y * p + x++] = 0; // skip 20
			case 0x4d: pixels[y * p + x++] = 0; // skip 19
			case 0x49: pixels[y * p + x++] = 0; // skip 18
			case 0x45: pixels[y * p + x++] = 0; // skip 17
			case 0x41: pixels[y * p + x++] = 0; // skip 16
			case 0x3d: pixels[y * p + x++] = 0; // skip 15
			case 0x39: pixels[y * p + x++] = 0; // skip 14
			case 0x35: pixels[y * p + x++] = 0; // skip 13
			case 0x31: pixels[y * p + x++] = 0; // skip 12
			case 0x2d: pixels[y * p + x++] = 0; // skip 11
			case 0x29: pixels[y * p + x++] = 0; // skip 10
			case 0x25: pixels[y * p + x++] = 0; // skip 9
			case 0x21: pixels[y * p + x++] = 0; // skip 8
			case 0x1d: pixels[y * p + x++] = 0; // skip 7
			case 0x19: pixels[y * p + x++] = 0; // skip 6
			case 0x15: pixels[y * p + x++] = 0; // skip 5
			case 0x11: pixels[y * p + x++] = 0; // skip 4
			case 0x0d: pixels[y * p + x++] = 0; // skip 3
			case 0x09: pixels[y * p + x++] = 0; // skip 2
			case 0x05: pixels[y * p + x++] = 0; // skip 1
				continue;
			}

			switch (command) {
			case 0x0f:
				i = w;
				break;
			case 0x0a:
				count = cmd_or_next(&cmd, 4);
				for (++cmd; count; --count) {
					pixels[y * p + x++] = *cmd + 0x10 * (player + 1);
				}
				dynamic = true;
				break;
			case 0x07:
				count = cmd_or_next(&cmd, 4);
				//dbgf("fill: %u pixels\n", count);
				for (++cmd; count; --count) {
					//dbgf(" %x", (unsigned)(*cmd) & 0xff);
					pixels[y * p + x++] = *cmd;
				}
				break;
			case 0x06:
				count = cmd_or_next(&cmd, 4);

				for (; count; --count)
					pixels[y * p + x++] = *++cmd + 0x10 * (player + 1);
				break;
			case 0x02:
				count = ((*cmd & 0xf0) << 4) + cmd[1];
				for (++cmd; count; --count)
					pixels[y * p + x++] = *++cmd;
				break;
			default:
				printf("unknown cmd at %X: %X, %X\n",
					(unsigned)(cmd - (const unsigned char*)slp.hdr),
					 *cmd, command
				);
				// dump some more bytes
				for (size_t i = 0; i < 16; ++i)
					printf(" %02hhX", *((const unsigned char*)cmd + i));
				puts("");
#if 1
				while (*cmd != 0xf)
					++cmd;
				i = w;
#endif
				break;
			}
		}

#if 0
		while (cmd[-1] != 0xff)
			++cmd;
#endif
	}

	bnds.w = info->width;
	bnds.h = info->height;
	return dynamic;
}

Animation::Animation(res_id id, const DRS &drs) : slp(drs.open_slp(id)), size(0), images(), id(id), image_count(0), dynamic(false) {
	if (memcmp(slp.hdr->version, "2.0N", 4))
		throw std::runtime_error("Could not load animation: bad header");

	images.reset(new Image[size = image_count = slp.hdr->frame_count]);

	for (int i = 0, n = slp.hdr->frame_count; i < n; ++i)
		if (images[i].load(slp, i)) {
			dynamic = true;
			break;
		}

	// use custom parsing if dynamic
	if (!dynamic)
		return;

	images.reset(new Image[size = image_count * io::max_players]);

	for (unsigned p = 0; p < io::max_players; ++p)
		for (unsigned i = 0; i < image_count; ++i)
			images[(unsigned long long)p * image_count + i].load(slp, i, p);
}


const Image &Animation::subimage(unsigned index, unsigned player) const {
	return dynamic ? images[(player % io::max_players) * (unsigned long long)image_count + index % this->image_count]
		: images[index % this->image_count];
}

bool operator<(const Animation &lhs, const Animation &rhs) { return lhs.id < rhs.id; }
bool operator<(const Image &lhs, const Image &rhs) { return lhs.id < rhs.id; }

Dialog::Dialog(res_id id, DialogSettings &&cfg, const DRS &drs, int bkgmode)
	: id(id), drs(drs), cfg(cfg), pal(drs.open_pal(this->cfg.pal), &SDL_FreePalette), bkgmode(bkgmode), bkganim(nullptr)
{
	set_bkg(bkgmode);
}

void Dialog::set_bkg(int bkgmode) {
	if (bkgmode == -1) {
		bkganim.reset();
		this->bkgmode = bkgmode;
		return;
	}

	assert(bkgmode < 3);
	bkganim.reset(new Animation(std::move(drs.open_anim(cfg.bmp[bkgmode]))));
}

DialogColors Dialog::colors() {
	DialogColors col;

	for (unsigned i = 0; i < 6; ++i) {
		col.bevel[i] = pal->colors[cfg.bevel[i]]; col.bevel[i].a = cfg.shade;
		col.text [i] = pal->colors[cfg.text [i]]; col.text [i].a = cfg.shade;
		col.focus[i] = pal->colors[cfg.focus[i]]; col.focus[i].a = cfg.shade;
		col.state[i] = pal->colors[cfg.state[i]]; col.state[i].a = cfg.shade;
	}

	return col;
}

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

bool Assets::open_item(DrsItem &item, res_id id, DrsType type) const noexcept {
	for (auto &b : blobs)
		if (b->open_item(item, id, type))
			return true;

	return false;
}

bool Assets::open_item(DrsItem &item, res_id id, DrsType type, unsigned &pos) const noexcept {
	for (unsigned i = 0, n = blobs.size(); i < n; ++i)
		if (blobs[i]->open_item(item, id, type)) {
			pos = i;
			return true;
		}

	return false;
}

SDL_Palette *Assets::open_pal(res_id id) const {
	DrsItem item; unsigned pos;

	if (!open_item(item, id, DrsType::binary, pos))
		throw std::runtime_error(std::string("Could not load palette ") + std::to_string(id));

	return blobs[pos]->open_pal(id);
}

}
