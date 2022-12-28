#include "../legacy.hpp"

#include <cassert>

#include <stdexcept>
#include <vector>

#include "endian.h"

#include "gfx.hpp"
#include "../string.hpp"

#include "../nominmax.hpp"

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

struct SlpFrameInfo final {
	uint32_t cmd_table_offset;
	uint32_t outline_table_offset;
	uint32_t palette_offset;
	uint32_t properties;
	int32_t width, height;
	int32_t hotspot_x, hotspot_y;
};

DRS::DRS(const std::string &path) : in(path, std::ios_base::binary), items() {
	in.exceptions(std::ofstream::failbit | std::ofstream::badbit);

	DrsHdr hdr{ 0 };
	in.read((char*)&hdr, sizeof(hdr));

	hdr.nlist = le16toh(hdr.nlist);
	hdr.listend = le16toh(hdr.listend);

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

	printf("items read: %llu\n", (unsigned long long)items.size());
}

std::vector<uint8_t> DRS::open_wav(DrsId k) {
	uint32_t id = (uint32_t)k;
	DrsItem key{ id, 0, 0 };
	auto it = items.find(key);
	if (it == items.end())
		throw std::runtime_error(std::string("invalid audio ID: ") + std::to_string(id));

	const DrsItem &item = *it;

	// fetch data
	in.seekg(item.offset);
	std::vector<uint8_t> data(item.size, 0);
	in.read((char*)data.data(), item.size);

	return data;
}

Slp DRS::open_slp(DrsId k) {
	uint32_t id = (uint32_t)k;
	DrsItem key{ id, 0, 0 };
	auto it = items.find(key);
	if (it == items.end())
		throw std::runtime_error(std::string("invalid Slp ID: ") + std::to_string(id));

	Slp slp;
	const DrsItem &item = *it;

	// define the memlayout
	struct SlpHdr {
		char version[4];
		int32_t frame_count;
		char comment[24];
	} hdr;

	// fetch data
	in.seekg(0, std::ios_base::end);
	long long end = in.tellg();

	in.seekg(item.offset);
	long long start = in.tellg();

	in.read((char*)&hdr, sizeof(hdr));

	// parse header
	cstrncpy(slp.version, hdr.version, 4);
	cstrncpy(slp.comment, hdr.comment, 24);

	int32_t frames = hdr.frame_count;

	std::vector<SlpFrameInfo> fi;

	// parse frames (if available)
	if (frames <= 0)
		return slp;

	fi.resize(frames);
	in.read((char*)fi.data(), frames * sizeof(SlpFrameInfo));

	for (auto &f : fi) {
		f.outline_table_offset = le32toh(f.outline_table_offset);
		f.cmd_table_offset = le32toh(f.cmd_table_offset);
	}

	// these containers help with estimating how big a frame really is
	std::vector<uint32_t> edge_offset, cmd_offset;

	for (auto &f : fi) {
		edge_offset.emplace_back(f.outline_table_offset);
		cmd_offset.emplace_back(f.cmd_table_offset);
	}

	std::sort(edge_offset.begin(), edge_offset.end());
	std::sort(cmd_offset.begin(), cmd_offset.end());

	for (SlpFrameInfo &f : fi) {
		SlpFrame &sf = slp.frames.emplace_back();

		if (f.width < 0 || f.height < 0)
			throw std::runtime_error("negative image dimensions");

		sf.w = f.width;
		sf.h = f.height;
		sf.hotspot_x = f.hotspot_x;
		sf.hotspot_y = f.hotspot_y;

		// read frame data
		// read frame row edge
		in.seekg(start + f.outline_table_offset);

		sf.frameEdges.resize(f.height);
		in.read((char*)sf.frameEdges.data(), f.height * sizeof(SlpFrameRowEdge));

		for (SlpFrameRowEdge &e : sf.frameEdges) {
			e.left_space = le16toh(e.left_space);
			e.right_space = le16toh(e.right_space);
		}

		long long from = in.tellg();

		// read cmd
		// since the size is not specified, we will have to guess
		// assume that the next outline_table_offset or cmd_table_offset ends it
		// as it's not possible to have overlapping frame info
		uint32_t size = 2u * f.width * f.height;

		auto it_edge_next = std::lower_bound(edge_offset.begin(), edge_offset.end(), f.outline_table_offset + 1);
		if (it_edge_next != edge_offset.end())
			size = std::min(size, *it_edge_next);

		auto it_cmd_next = std::lower_bound(cmd_offset.begin(), cmd_offset.end(), f.cmd_table_offset + 1);
		if (it_cmd_next != cmd_offset.end())
			size = std::min(size, *it_cmd_next);

		in.seekg(start + f.cmd_table_offset + 4 * f.height);
		size = std::min<unsigned>(size, (unsigned)(end - in.tellg()));

		sf.cmd.resize(size);
		in.read((char*)sf.cmd.data(), size);
	}

	return slp;
}

static const char JASC_PAL[] = {
	"JASC-PAL\r\n"
	"0100\r\n"
	"256\r\n"
};

std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> DRS::open_pal(DrsId k) {
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

int ::rand(void);

namespace gfx {

using namespace aoe::io;

static unsigned cmd_or_next(const std::vector<uint8_t> &data, uint32_t &cmdpos, unsigned n)
{
	unsigned v = data.at(cmdpos) >> n;
	return v ? v : data.at(++cmdpos);
}

bool Image::load(const SDL_Palette *pal, const Slp &slp, unsigned index, unsigned player) {
	const SlpFrame &frame = slp.frames.at(index);

	hotspot_x = frame.hotspot_x;
	hotspot_y = frame.hotspot_y;

	surface.reset(SDL_CreateRGBSurface(0, frame.w, frame.h, 8, 0, 0, 0, 0));
	if (!surface.get())
		throw std::runtime_error(std::string("Could not create Slp surface: ") + SDL_GetError());

	// load pixel data
	unsigned char *pixels = (unsigned char*)surface->pixels;

	if (SDL_SetSurfacePalette(surface.get(), (SDL_Palette*)pal))
		throw std::runtime_error(std::string("Could not set Slp palette: ") + SDL_GetError());

	if (surface->format->format != SDL_PIXELFORMAT_INDEX8)
		throw std::runtime_error(std::string("Unexpected image format: ") + SDL_GetPixelFormatName(surface->format->format));

	uint32_t cmdpos = 0;
	const std::vector<uint8_t> &cmd = frame.cmd;
	unsigned maxerr = 5;
	bool bail_out = false, dynamic = false;

	for (int y = 0, h = frame.h; y < h; ++y) {
		const SlpFrameRowEdge &e = frame.frameEdges.at(y);

		// check if row is non-zero
		if (e.left_space == invalid_edge || e.right_space == invalid_edge)
			continue;

		int line_size = frame.w - e.left_space - e.right_space;

		// fill row with garbage so any funny bytes will be visible immediately
		for (int x = e.left_space, w = x + line_size, p = surface->pitch; x < w; ++x)
			pixels[y * p + x] = rand();

		for (int i = e.left_space, x = i, w = x + line_size, p = surface->pitch; i <= w; ++i, ++cmdpos) {
			unsigned char bc = cmd.at(cmdpos), count = 0;

			switch (bc & 0xf) {
			case 0x0f:
				i = w;
				continue;
			case 0x07:
				count = cmd_or_next(cmd, cmdpos, 4);

				for (++cmdpos; count; --count)
					pixels[y * p + x++] = cmd.at(cmdpos);

				break;
			case 0x06:
				count = cmd_or_next(cmd, cmdpos, 4);

				for (; count; --count)
					pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1);

				dynamic = true;
				break;
			default:
				switch (bc) {
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
					++cmdpos;
					continue;
					// player color fill
				case 0xF6: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 15
				case 0xE6: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 14
				case 0xD6: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 13
				case 0xC6: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 12
				case 0xB6: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 11
				case 0xA6: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 10
				case 0x96: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 9
				case 0x86: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 8
				case 0x76: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 7
				case 0x66: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 6
				case 0x56: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 5
				case 0x46: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 4
				case 0x36: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 3
				case 0x26: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 2
				case 0x16: pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1); // player fill 1
					dynamic = true;
					continue;
					// XXX pixel count if lower_nibble == 4: 1 + cmd >> 2
				case 0xfc: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 63
				case 0xf8: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 62
				case 0xf4: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 61
				case 0xf0: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 60
				case 0xec: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 59
				case 0xe8: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 58
				case 0xe4: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 57
				case 0xe0: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 56
				case 0xdc: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 55
				case 0xd8: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 54
				case 0xd4: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 53
				case 0xd0: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 52
				case 0xcc: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 51
				case 0xc8: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 50
				case 0xc4: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 49
				case 0xc0: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 48
				case 0xbc: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 47
				case 0xb8: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 46
				case 0xb4: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 45
				case 0xb0: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 44
				case 0xac: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 43
				case 0xa8: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 42
				case 0xa4: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 41
				case 0xa0: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 40
				case 0x9c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 39
				case 0x98: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 38
				case 0x94: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 37
				case 0x90: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 36
				case 0x8c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 35
				case 0x88: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 34
				case 0x84: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 33
				case 0x80: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 32
				case 0x7c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 31
				case 0x78: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 30
				case 0x74: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 29
				case 0x70: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 28
				case 0x6c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 27
				case 0x68: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 26
				case 0x64: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 25
				case 0x60: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 24
				case 0x5c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 23
				case 0x58: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 22
				case 0x54: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 21
				case 0x50: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 20
				case 0x4c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 19
				case 0x48: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 18
				case 0x44: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 17
				case 0x40: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 16
				case 0x3c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 15
				case 0x38: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 14
				case 0x34: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 13
				case 0x30: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 12
				case 0x2c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 11
				case 0x28: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 10
				case 0x24: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 9
				case 0x20: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 8
				case 0x1c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 7
				case 0x18: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 6
				case 0x14: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 5
				case 0x10: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 4
				case 0x0c: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 3
				case 0x08: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 2
				case 0x04: pixels[y * p + x++] = cmd.at(++cmdpos); // fill 1
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
				default:
					break;
				}

				if (maxerr) {
					fprintf(stderr, "%s: unknown cmd at %X: %X\n", __func__, cmdpos, bc);
					--maxerr;
				} else if (!bail_out) {
					bail_out = true;
					fprintf(stderr, "%s: too many errors\n", __func__);
				}

#if 1
				while (cmd.at(cmdpos) != 0x0f)
					++cmdpos;

				i = w;
#endif
				break;
			}
		}
	}

	if (SDL_SetColorKey(surface.get(), SDL_TRUE, 0))
		fprintf(stderr, "Could not set transparency: %s\n", SDL_GetError());

	return dynamic;
}

}

}
