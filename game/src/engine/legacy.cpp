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
	in.seekg(item.offset);
	std::streampos start = in.tellg();

	in.read((char*)&hdr, sizeof(hdr));

	// parse header
	cstrncpy(slp.version, hdr.version, 4);
	cstrncpy(slp.comment, hdr.comment, 24);

	int32_t frames = hdr.frame_count;

	std::vector<SlpFrameInfo> fi;

	// parse frames (if available)
	if (frames > 0) {
		fi.resize(frames);
		in.read((char*)fi.data(), frames * sizeof(SlpFrameInfo));
	}

	// these containers help with estimating how big a frame really is
	std::vector<uint32_t> edge_offset, cmd_offset;

	for (auto &f : fi) {
		edge_offset.emplace_back(f.outline_table_offset);
		cmd_offset.emplace_back(f.cmd_table_offset);
	}

	edge_offset.emplace_back(item.size);
	cmd_offset.emplace_back(item.size);

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
		in.seekg(start);
		in.seekg(f.outline_table_offset, std::ios_base::cur);

		sf.frameEdges.resize(f.height);
		in.read((char*)sf.frameEdges.data(), f.height * sizeof(SlpFrameRowEdge));

		// read cmd
		// since the size is not specified, we will have to guess
		// assume that the next outline_table_offset or cmd_table_offset ends it
		// as it's not possible to have overlapping frame info
		uint32_t end = std::min(2u * f.width * f.height, item.size);

		auto it_edge_next = std::lower_bound(edge_offset.begin(), edge_offset.end(), f.outline_table_offset + 1);
		if (it_edge_next != edge_offset.end())
			end = std::min(end, *it_edge_next);

		auto it_cmd_next = std::lower_bound(cmd_offset.begin(), cmd_offset.end(), f.cmd_table_offset + 1);
		if (it_cmd_next != cmd_offset.end())
			end = std::min(end, *it_cmd_next);

		uint32_t size = end + 1;

		in.seekg(start);
		in.seekg(f.cmd_table_offset, std::ios_base::cur);
		in.seekg(4 * f.height, std::ios_base::cur);

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
	bool bail_out = false;

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
			unsigned command, count;

			command = cmd.at(cmdpos) & 0x0f;

			switch (command) {
			case 0x0f:
				i = w;
				break;
			case 0x07:
				count = cmd_or_next(cmd, cmdpos, 4);

				for (++cmdpos; count; --count)
					pixels[y * p + x++] = cmd.at(cmdpos);

				break;
			default:
				if (maxerr) {
					fprintf(stderr, "%s: unknown cmd at %X: %X\n", __func__, cmdpos, command);
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

	return false;
}

}

}
