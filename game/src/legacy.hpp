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

enum class BldIcon {
	academy,
	archery1,
	archery2,
	barracks1,
	barracks2,
	barracks3,
	dock1,
	dock2,
	dock3,
	siege_workshop,
	farm,
	granary1,
	granary2,
	tower3,
	tower2,
	house1,
	house2,
	house3,
	house4,
	government1,
	government2,
	market1,
	market2,
	market3,
	stable2,
	stable1,
	tower1,
	wonder,
	storage1,
	storage2,
	storage3,
	temple1,
	temple2,
	towncenter1,
	towncenter2,
	towncenter3,
	towncenter4,
	towncenter5,
	towncenter6,
	tradeworkshop,
	wall1,
	wall2,
	wall3,
	tower4,
};

enum class UnitIcon {
	villager,
	priest,
	melee1, // clubman
	melee2, // axeman
	melee3,
	melee4,
	archer1,
	archer2,
	archer3,
	siege3,
	chariot,
	cavalry2,
	heavy_cavalry,
	cavalry3,
	siege1,
	siege2,
	heavy_melee1,
	heavy_melee2,
	fisher1,
	tradeboat1,
	fisher2,
	transport1,
	warship1,
	tradeboat2,
	warship2,
	transport2,
	warship3,
	melee5,
	archer4,
	archer5,
	warship4,
	cavalry1,
	horse,
	elephant,
	fish,
	wood,
	food,
	deer,
	stone,
	gold,
	lion,
	alligator,
	artefact1,
	artefact2,
	artefact3,
	artefact4,
	artefact5,
	heavy_archer,
	archer6,
	melee6,
	heavy_melee3,
	siege4,
	heavy_warship,
	siege5,
	cavalry4,
	tower,
};

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

	gif_bird1 = 404,
	gif_bird1_shadow = 405,
	gif_bird1_glide = 406,
	gif_bird1_glide_shadow = 407,

	gif_bird2 = 518,
	gif_bird2_shadow = 519,
	gif_bird2_glide = 521,
	gif_bird2_glide_shadow = 520,

	gif_villager_stand = 418,
	gif_villager_move = 657,
	gif_villager_attack = 224,
	gif_villager_die1 = 314,
	gif_villager_die2 = 315,
	gif_villager_decay = 373,

	gif_building_icons = 50704,
	gif_unit_icons = 50730,

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

	sfx_villager1 = 5037,
	sfx_villager2 = 5053,
	sfx_villager3 = 5054,
	sfx_villager4 = 5158,
	sfx_villager5 = 5171,
	sfx_villager6 = 5205,
	sfx_villager7 = 5206,

	sfx_villager_die1 = 5055,
	sfx_villager_die2 = 5056,
	sfx_villager_die3 = 5057,
	sfx_villager_die4 = 5058,
	sfx_villager_die5 = 5059,
	sfx_villager_die6 = 5060,
	sfx_villager_die7 = 5061,
	sfx_villager_die8 = 5062,
	sfx_villager_die9 = 5063,
	sfx_villager_die10 = 5064,

	sfx_player_resign = 50306,
	sfx_gameover_defeat = 50321,
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
