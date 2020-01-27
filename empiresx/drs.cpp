#include "drs.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
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

bool io::DrsHdr::good() const {
	return strncmp(version, "1.00tribe", strlen("1.00tribe")) == 0;
}

DRS::DRS(const std::string &name, iofd fd, bool map) : blob(name, fd, map), hdr((io::DrsHdr*)blob.get()) {
	// verify header
	if (!hdr->good())
		throw std::runtime_error(std::string("Bad DRS file \"") + name + "\": invalid header");
}

bool DRS::open_item(io::DrsItem &item, res_id id, uint32_t type) {
	const IO_DrsList *list = (IO_DrsList*)&hdr[1];

	for (unsigned i = 0; i < hdr->nlist; ++i) {
		const io::DrsItem *entry = (io::DrsItem*)((char*)hdr + list[i].offset);

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

static unsigned cmd_or_next(const unsigned char **cmd, unsigned n)
{
	const unsigned char *ptr = *cmd;
	unsigned v = *ptr >> n;
	if (!v)
		v = *(++ptr);
	*cmd = ptr;
	return v;
}

bool slp_read(SDL_Surface *surface, const Palette &p, const Slp &slp, unsigned player) {
	bool dynamic = false;

	SDL_Palette *rawpal;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal(NULL, &SDL_FreePalette);

	// FIXME refactor Palette to SDL_Palette to prevent converting this...
	if (!(rawpal = SDL_AllocPalette(256)))
		throw std::runtime_error(std::string("Could not read Slp data: ") + SDL_GetError());

	pal.reset(rawpal);

	for (unsigned i = 0; i < 256; ++i)
		pal->colors[i] = p.tbl[i];

	if (!surface || SDL_SetSurfacePalette(surface, rawpal))
		throw std::runtime_error(std::string("Could not set Slp palette: ") + SDL_GetError());

	if (surface->format->format != SDL_PIXELFORMAT_INDEX8)
		throw std::runtime_error(std::string("Unexpected image format: ") + SDL_GetPixelFormatName(surface->format->format));

	unsigned char *pixels = (unsigned char*)surface->pixels;

	// clear pixel data
	for (int y = 0, h = slp.info->height, p = surface->pitch; y < h; ++y)
		for (int x = 0, w = slp.info->width; x < w; ++x)
			pixels[y * p + x] = 0;

	const io::SlpFrameRowEdge *edge = (io::SlpFrameRowEdge*)((char*)slp.hdr + slp.info->outline_table_offset);
	const unsigned char *cmd = (unsigned char*)((char*)slp.hdr + slp.info->cmd_table_offset + 4 * slp.info->height);

	for (int y = 0, h = slp.info->height; y < h; ++y, ++edge) {
		if (edge->left_space == io::invalid_edge || edge->right_space == io::invalid_edge)
			continue;

		int line_size = slp.info->width - edge->left_space - edge->right_space;

		// fill line_size with default value
		for (int x = edge->left_space, w = x + line_size, p = surface->pitch; x < w; ++x)
			pixels[y * p + x] = rand();

		for (int i = edge->left_space, x = i, w = x + line_size, p = surface->pitch; i <= w; ++i, ++cmd) {
			unsigned command, count;

			command = *cmd & 0x0f;

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

	if (SDL_SetColorKey(surface, SDL_TRUE, 0))
		fprintf(stderr, "Could not set transparency: %s\n", SDL_GetError());

	return dynamic;
}

Image::Image() : surface((SDL_Surface*)NULL), texture(0, 0, NULL), hotspot_x(0), hotspot_y(0) {}

bool Image::load(SimpleRender &r, const Palette &pal, const Slp &slp, unsigned player) {
	bool dynamic = false;

	hotspot_x = slp.info->hotspot_x;
	hotspot_y = slp.info->hotspot_y;

	surface.reset(SDL_CreateRGBSurface(0, slp.info->width, slp.info->height, 8, 0, 0, 0, 0));
	if (!surface.data())
		throw std::runtime_error(std::string("Could not create Slp surface: ") + SDL_GetError());

	dynamic = slp_read(surface.data(), pal, slp, player);
	texture.reset(r, surface.data());

	return dynamic;
}

void Image::draw(SimpleRender &r, int x, int y, int w, int h) {
	if (!w) w = surface.data()->w;
	if (!h) h = surface.data()->h;
	texture.paint(r, x - hotspot_x, y - hotspot_y, w, h);
}

Palette DRS::open_pal(res_id id) {
	io::DrsItem item;

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
	io::DrsItem item;

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
