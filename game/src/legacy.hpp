#pragma once

#include <cstdint>
#include <string>
#include <set>
#include <fstream>
#include <vector>

#include "nominmax.hpp"

namespace aoe {

static constexpr unsigned WINDOW_WIDTH_MIN = 640, WINDOW_HEIGHT_MIN = 480;
static constexpr unsigned DEFAULT_TICKS_PER_SECOND = 30;

namespace io {

enum DrsId final {
	bkg_main_menu = 50051,
	bkg_achievements = 50061,
	bkg_defeat = 50058,

	sfx_ui_click = 5035,
	sfx_priest_convert2 = 5051,
};

struct DrsBkg final {
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
	unsigned bevel_col[6], text_col[6], focus_col[6], state_col[6];
};

/** Raw uncompressed game asset. */
struct DrsItem final {
	uint32_t id;     /**< Unique resource identifier. */
	uint32_t offset; /**< Raw file offset. */
	uint32_t size;   /**< Size in bytes. */

	friend bool operator<(const DrsItem &lhs, const DrsItem &rhs) { return lhs.id < rhs.id; }
};

/** Data resource set */
class DRS final {
	std::ifstream in;
	std::set<DrsItem> items;
public:
	DRS(const std::string &path);
	DRS(const DRS&) = delete;
	DRS(DRS&&) = delete;

	DrsBkg open_bkg(DrsId id);
	std::vector<uint8_t> open_wav(DrsId id);
};

}

}
