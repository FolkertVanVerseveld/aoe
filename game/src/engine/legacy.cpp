#include "../legacy.hpp"

#include <cassert>

#include <vector>

#include "endian.h"

#include "gfx.hpp"

#include <except.hpp>
#include <minmax.hpp>

#include <tracy/Tracy.hpp>

namespace aoe {

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
