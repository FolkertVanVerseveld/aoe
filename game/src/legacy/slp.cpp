#include "../legacy.hpp"

#include "../debug.hpp"
#include "../string.hpp"

#include "../engine/endian.h"

#include <except.hpp>
#include <minmax.hpp>

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

}
