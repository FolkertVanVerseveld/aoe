#include "../legacy.hpp"

#include <cassert>

#include <stdexcept>
#include <vector>

#include "endian.h"

#include "gfx.hpp"
#include "../string.hpp"

#include "../nominmax.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace io {

struct SlpFrameInfo final {
	uint32_t cmd_table_offset;
	uint32_t outline_table_offset;
	uint32_t palette_offset;
	uint32_t properties;
	int32_t width, height;
	int32_t hotspot_x, hotspot_y;
};

Slp DRS::open_slp(DrsId k) {
	ZoneScoped;
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

	{
		ZoneScopedN("parse frames");
		// parse frames (if available)
		if (frames <= 0)
			return slp;

		fi.resize(frames);
		in.read((char*)fi.data(), frames * sizeof(SlpFrameInfo));

		for (auto &f : fi) {
			f.outline_table_offset = le32toh(f.outline_table_offset);
			f.cmd_table_offset = le32toh(f.cmd_table_offset);
		}
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
		ZoneScopedN("frame info");
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
	ZoneScoped;
	const SlpFrame &frame = slp.frames.at(index);

	assert(player < MAX_PLAYERS);

	if (!player)
		player = 9;
	else
		--player;

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
	bool bail_out = false, dynamic = false, rectangular = true;

	mask.clear();

	for (int y = 0, h = frame.h; y < h; ++y) {
		const SlpFrameRowEdge &e = frame.frameEdges.at(y);

		// check if row is non-zero
		if (e.left_space == invalid_edge || e.right_space == invalid_edge) {
			mask.emplace_back(0, 0);
			rectangular = false;
			continue;
		}

		int line_size = frame.w - e.left_space - e.right_space;

		if (line_size < frame.w)
			rectangular = false;

		mask.emplace_back(e.left_space, e.left_space + line_size);

		// fill row with garbage so any funny bytes will be visible immediately
		for (int x = e.left_space, w = x + line_size, p = surface->pitch; x < w; ++x)
			pixels[y * p + x] = rand();

		for (int i = e.left_space, x = i, w = x + line_size, p = surface->pitch; i <= w; ++i, ++cmdpos) {
			unsigned char bc = cmd.at(cmdpos), count = 0;

			switch (bc & 0xf) {
				// eol
			case 0x0f:
				i = w;
				continue;
				// player fill
			case 0x0a:
				count = cmd_or_next(cmd, cmdpos, 4);

				for (++cmdpos; count; --count)
					pixels[y * p + x++] = cmd.at(cmdpos) + 0x10 * (player + 1);

				dynamic = true;
				break;
				// block fill
			case 0x07:
				count = cmd_or_next(cmd, cmdpos, 4);

				for (++cmdpos; count; --count)
					pixels[y * p + x++] = cmd.at(cmdpos);

				break;
				// large player fill
			case 0x06:
				count = cmd_or_next(cmd, cmdpos, 4);

				for (; count; --count)
					pixels[y * p + x++] = cmd.at(++cmdpos) + 0x10 * (player + 1);

				dynamic = true;
				break;
				// large skip
			case 0x03:
				for (count = cmd.at(++cmdpos); count; --count)
					pixels[y * p + x++] = 0;

				continue;
				// large fill
			case 0x02:
				count = ((cmd.at(cmdpos) & 0xf0) << 4) + cmd.at(cmdpos + 1);

				for (++cmdpos; count; --count)
					pixels[y * p + x++] = cmd.at(++cmdpos);

				break;
				// block skip
			case 0x0d:
			case 0x0b:
			case 0x09:
			case 0x05:
			case 0x01:
				count = cmd_or_next(cmd, cmdpos, 2);

				for (; count; --count)
					pixels[y * p + x++] = 0;

				break;
				// fill
			case 0x0c:
			case 0x08:
			case 0x04:
			case 0x00:
				count = cmd_or_next(cmd, cmdpos, 2);

				for (; count; --count)
					pixels[y * p + x++] = cmd.at(++cmdpos);

				break;
			case 0x0e:
				switch (bc & 0xf0) {
				case 0x40:
				case 0x60:
					// TODO figure out what the special color is, for now, just use the next byte as color
					pixels[y * p + x++] = cmd.at(++cmdpos);
					continue;
				default: // oops
					break;
				}
				// FALL THROUGH
			default: // oops
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

	// delete mask if all rows are equal to image size
	if (rectangular)
		mask.clear();

	if (SDL_SetColorKey(surface.get(), SDL_TRUE, 0))
		fprintf(stderr, "Could not set transparency: %s\n", SDL_GetError());

	return dynamic;
}

}

}
