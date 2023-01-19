#pragma once

#include <cstdint>
#include <string>
#include <set>
#include <fstream>
#include <vector>
#include <memory>
#include <map>

#include "nominmax.hpp"

#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_surface.h>

#include "legacy/strings.hpp"

namespace aoe {

static constexpr unsigned WINDOW_WIDTH_MIN = 640, WINDOW_HEIGHT_MIN = 480;
static constexpr unsigned WINDOW_WIDTH_MAX = 1024, WINDOW_HEIGHT_MAX = 768;
static constexpr unsigned DEFAULT_TICKS_PER_SECOND = 30;
static constexpr unsigned MAX_PLAYERS = 9; // 8 + 1 for gaia

namespace io {

enum class DrsId {
	bkg_main_menu = 50051, // 50051
	bkg_singleplayer = 50052,
	bkg_multiplayer = 50053,
	bkg_editor_menu = 50054,
	bkg_victory = 50057,
	bkg_defeat = 50058,
	bkg_mission = 50060,
	bkg_achievements = 50061,

	img_editor = 50129,

	trn_desert = 15000,
	trn_grass = 15001,
	trn_water = 15002,
	trn_deepwater = 15003,

	pal_default = 50500,

	img_dialog0 = 50210,

	gif_menubar0 = 50741,
	gif_cursors = 51000,

	gif_villager_stand = 418,
	gif_villager_move = 657,
	gif_villager_attack = 224,
	gif_villager_die1 = 314,
	gif_villager_die2 = 315,
	gif_villager_decay = 373,

	bld_town_center = 280,
	bld_town_center_player = 230,
	bld_barracks = 254,

	sfx_ui_click = 5035,
	sfx_priest_convert2 = 5051,
	sfx_chat = 50302,
	sfx_towncenter = 5044,
	sfx_barracks = 5022,

	sfx_bld_die1 = 5077,
	sfx_bld_die2 = 5078,
	sfx_bld_die3 = 5079,

	sfx_player_resign = 50306,
};

struct DrsBkg final {
	char bkg_name1[16], bkg_name2[16], bkg_name3[16];
	int bkg_id[3];
	char pal_name[16];
	unsigned pal_id;
	char cur_name[16];
	unsigned cur_id;
	unsigned shade;
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

constexpr int16_t invalid_edge = INT16_MIN;

/** Game specific image file format subimage boundaries. */
struct SlpFrameRowEdge final {
	int16_t left_space;
	int16_t right_space;
};

struct SlpFrame final {
	int w, h;
	int hotspot_x, hotspot_y;
	std::vector<SlpFrameRowEdge> frameEdges;
	std::vector<uint8_t> cmd;
};

class Slp final {
public:
	std::string version, comment;
	std::vector<SlpFrame> frames;
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
	Slp open_slp(DrsId id);
	std::vector<uint8_t> open_wav(DrsId id);
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> open_pal(DrsId id);
};

enum class PE_Type {
	unknown = 0,
	mz = 1,
	dos = 2,
	pe = 3,
	peopt = 4,
};

enum class RsrcType {
	unknown = 0,
	cursor = 1,
	bitmap = 2,
	icon = 3,
	menu = 4,
	dialog = 5,
	string = 6,
	fontdir = 7,
	font = 8,
	accelerator = 9,
	rcdata = 10,
	messagetable = 11,
	group_cursor = 12,
	group_icon = 14,
	version = 16,
	dlginclude = 17,
	plugplay = 19,
	vxd = 20,
	anicursor = 21,
	aniicon = 22,
};

struct sechdr final {
	char s_name[8];
	uint32_t s_paddr;
	uint32_t s_vaddr;
	uint32_t s_size;
	uint32_t s_scnptr;
	uint32_t s_relptr;
	uint32_t s_lnnoptr;
	uint16_t s_nreloc;
	uint16_t s_nlnno;
	uint32_t s_flags;
};

typedef uint16_t res_id;

class PE final {
	std::ifstream in;
	PE_Type m_type;
	unsigned bits;
	unsigned nrvasz;
	std::vector<sechdr> sections;
	std::string path;
public:
	PE(const std::string &path);

	PE_Type type() const noexcept { return m_type; }

	bool load_res(RsrcType type, res_id, size_t &pos, size_t &size);
	std::string load_string(res_id);

	void read(char *dst, size_t pos, size_t size);
};

class LanguageData final {
public:
	std::map<std::string, std::vector<std::string>> civs;
	std::map<StrId, std::string> tbl;

	void load(PE &dll);

	void collect_civs(std::vector<std::string>&);

	std::string find(StrId);
	/** Try to localise text given by \a id or return \a def when not found. */
	std::string find(StrId id, const std::string &def);
private:
	void civ_add(PE &dll, res_id&, StrId);
};

}

}
