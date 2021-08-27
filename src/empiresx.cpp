/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

// dear imgui: standalone example application for SDL2 + OpenGL
 // If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
 // (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
 // (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#pragma warning (disable: 4996)

#define IMGUI_INCLUDE_IMGUI_USER_H 1

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl2.h"

// addons
#include "imgui/imgui_memory_editor.h"
#include "imgui/ImGuiFileBrowser.h"

#include "../tracy/Tracy.hpp"

#include "lang.hpp"
#include "drs.hpp"
#include "math.hpp"
#include "graphics.hpp"
#include "engine.hpp"

#include "ui/all.hpp"

#include <cstdio>
#include <ctime>
#include <cstring>
#include <cmath>
#include <inttypes.h>

#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <variant>
#include <utility>
#include <memory>
#include <future>
#include <algorithm>
#include <stack>
#include <exception>
#include <stdexcept>
#include <map>

#include <signal.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_mixer.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define FDLG_CHOOSE_DIR "Fdlg Choose AoE dir"
#define FDLG_CHOOSE_SCN "Fdlg Choose SCN"
#define MSG_INIT "Msg Init"

#define MODAL_LOADING "Loading..."
#define MODAL_GLOBAL_SETTTINGS "Global settings"
#define MODAL_ABOUT "About game"
#define MODAL_WIP "Work in progress"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

static SDL_Window *window;
static SDL_GLContext gl_context;

namespace genie {

GLint max_texture_size;
std::thread::id t_main;

}

static const SDL_Rect dim_lgy[] = {
	{0, 0, 640, 480},
	{0, 0, 800, 600},
	{0, 0, 1024, 768},
};

static const char *str_dim_lgy[] = {
	"640x480", "800x600", "1024x768", "Custom",
};

#pragma warning(disable : 26812)

static bool show_debug;
static bool fs_has_root;
std::atomic_bool running(true);

enum class SubMenuId {
	singleplayer_menu,
	singleplayer_game,
};

genie::MenuId menu_id = genie::MenuId::config;
SubMenuId submenu_id = SubMenuId::singleplayer_menu;

static bool constexpr menu_is_game(genie::MenuId id=menu_id) {
	return id == genie::MenuId::scn_edit;
}

static const unsigned anim_counts[] = {
	7,
	1,
};

static void anim_add(std::map<genie::AnimationId, unsigned> &anims, genie::TilesheetBuilder &bld, unsigned idx, genie::AnimationId anim, genie::DrsId id, SDL_Palette *pal)
{
	unsigned pos = bld.add(idx, id, pal);
	anims.emplace(anim, pos);
}

namespace genie {

AoE *eng = nullptr;

Assets assets;

Worker::Worker(GLchannel &ch) : tasks(WorkTaskType::stop, 10), results(WorkResult{ WorkTaskType::stop, WorkResultType::stop }, 10), ch(ch), mut(), p() {}

int Worker::run() {
	try {
		while (tasks.is_open()) {
			WorkTask task = tasks.consume();

			switch (task.type) {
				case WorkTaskType::stop:
					continue;
				case WorkTaskType::check_root:
					check_root(std::get<FdlgItem>(task.data));
					break;
				case WorkTaskType::load_menu:
					load_menu(task);
					break;
				case WorkTaskType::play_music:
					genie::jukebox.play(std::get<genie::MusicId>(task.data));
					break;
				default:
					assert("bad task type" == 0);
					break;
			}
		}
	} catch (WorkInterrupt&) {
		done(WorkTaskType::stop, WorkResultType::stop);
		return 1;
	}

	return 0;
}

bool Worker::progress(WorkerProgress &p) {
	std::unique_lock<std::mutex> lock(mut);

	unsigned v = this->p.step.load();

	p.desc = this->p.desc;
	p.step.store(v);
	p.total = this->p.total;

	return v != p.total;
}

void Worker::start(unsigned total, unsigned step) {
	std::unique_lock<std::mutex> lock(mut);
	if (!running)
		throw WorkInterrupt();
	p.step = step;
	p.total = total;
}

void Worker::set_desc(const std::string &s) {
	std::unique_lock<std::mutex> lock(mut);
	if (!running)
		throw WorkInterrupt();
	p.desc = s;
}

void Worker::load_menu(WorkTask &task) {
	MenuId id = std::get<MenuId>(task.data);

	start(4);
	set_desc("Loading menu dialog");

	// find corresponding dialog
	const genie::res_id dlg_ids[] = {
		(genie::res_id)-1,
		(genie::res_id)genie::DrsId::menu_start,
		(genie::res_id)genie::DrsId::menu_singleplayer,
		(genie::res_id)genie::DrsId::menu_editor,
		(genie::res_id)genie::DrsId::menu_scn_edit,
	};

	genie::Dialog dlg(genie::assets.blobs[2]->open_dlg(dlg_ids[(unsigned)id]));
	++p.step;
	set_desc("Loading backgrounds");

	genie::TilesheetBuilder bld;

	// add all backgrounds
	for (int i = 0; i < IM_ARRAYSIZE(dim_lgy); ++i) {
		dlg.set_bkg(i);
		genie::Animation &anim = *dlg.bkganim.get();
		bld.emplace(anim.subimage(0), dlg.pal.get());
	}

	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal(genie::assets.open_pal(genie::DrsId::palette), SDL_FreePalette);
	//bld.add(2, genie::DrsId::cursors, pal.get());

	++p.step;
	set_desc("Loading animations");

	std::map<AnimationId, unsigned> anims;

	// add any other images depending on the menu being loaded
	// anim_counts should match number of bld.add lines!
	switch (id) {
		case MenuId::scn_edit:
		{
			anim_add(anims, bld, 4, AnimationId::desert, genie::DrsId::terrain_desert, pal.get());
			anim_add(anims, bld, 4, AnimationId::grass, genie::DrsId::terrain_grass, pal.get());
			anim_add(anims, bld, 4, AnimationId::water_shallow, genie::DrsId::terrain_water_shallow, pal.get());
			anim_add(anims, bld, 4, AnimationId::water_deep, genie::DrsId::terrain_water_deep, pal.get());
			anim_add(anims, bld, 0, AnimationId::corner_water_desert, genie::DrsId::corner_desert_water, pal.get());
			anim_add(anims, bld, 0, AnimationId::corner_desert_grass, genie::DrsId::corner_desert_grass, pal.get());
			anim_add(anims, bld, 0, AnimationId::overlay_water, genie::DrsId::overlay_water, pal.get());
			break;
		}
		case MenuId::singleplayer:
		{
			std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> ui_pal(genie::assets.open_pal(genie::DrsId::ui_palette), SDL_FreePalette);

			anim_add(anims, bld, 2, AnimationId::ui_buttons, genie::DrsId::ui_buttons, ui_pal.get());
			break;
		}
	}


	// send back to main thread
	genie::Tilesheet ts(bld);
	ch.cmds.produce(GLcmdType::set_bkg, ts);
	ch.cmds.produce(GLcmdType::set_dlgcol, dlg.colors());
	ch.cmds.produce(GLcmdType::set_anims, anims);

	++p.step;
	set_desc("Loading menu music");

	switch (id) {
		case MenuId::start:
			genie::jukebox.play(genie::MusicId::open);
			break;
		case MenuId::editor:
		case MenuId::singleplayer:
			break;
		case MenuId::scn_edit:
			genie::jukebox.stop();
			break;
		default:
			assert("bad menu id" == 0);
			done(task, WorkResultType::fail);
			return;
	}

	++p.step;
	done(task, WorkResultType::success);
}

void Worker::check_root(const FdlgItem &item) {
#define COUNT 5

	genie::iofd fd[COUNT];
	// windows doesn't care about case sensitivity, but we need to make sure this also works on *n*x
	std::string fnames[COUNT] = {
		"Border.drs",
		"graphics.drs",
		"Interfac.drs",
		"sounds.drs",
		"Terrain.drs"
	};

	std::string ttfnames[] = {
		"arial.ttf",
		"arialbd.ttf",
		"comic.ttf",
		"comicbd.ttf",
		"COPRGTB.TTF",
		"COPRGTL.TTF",
	};

	struct ResChk final {
		genie::res_id id;
		unsigned type;
	};

	// just list some important assests to take an educated guess if everything looks fine
	const std::vector<std::vector<ResChk>> res = {
		{ // border
			{20000, 2}, {20001, 2}, {20002, 2}, {20003, 2}, {20004, 2}, {20006, 2},
		},{ // graphics
			{230, 2}, {254, 2}, {280, 2}, {418, 2}, {425, 2}, {463, 2},
		},{ // interface
			{50057, 0}, {50058, 0}, {50060, 0}, {50061, 0}, {50721, 2}, {50731, 2}, {50300, 3}, {50302, 3}, {50303, 3}, {50320, 3}, {50321, 3}, {50500, 0},
		},{ // sounds
			{5036, 3}, {5092, 3}, {5107, 3},
		},{ // terrain
			{15000, 2}, {15001, 2}, {15002, 2}, {15003, 2},
		}
	};

	start(3);
	set_desc(TXT(TextId::work_drs));

	std::string dir(item.first);

	try {
		for (unsigned i = 0; i < COUNT; ++i) {
			size_t off;
			std::string path(genie::drs_path(item.first, fnames[i], off));
			if ((fd[i] = genie::open(path.c_str(), off)) == genie::fd_invalid) {
				for (unsigned j = 0; j < i; ++j)
					genie::close(fd[j]);

				throw std::runtime_error(path);
			}
		}
	} catch (std::runtime_error &e) {
		if (item.first == item.second) {
			done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_path, e.what()));
			return;
		}
		try {
			dir = item.second;

			for (unsigned i = 0; i < COUNT; ++i) {
				size_t off;
				std::string path(genie::drs_path(item.second, fnames[i], off));
				if ((fd[i] = genie::open(path.c_str(), off)) == genie::fd_invalid) {
					for (unsigned j = 0; j < i; ++j)
						genie::close(fd[j]);

					throw std::runtime_error(path);
				}
			}
		} catch (std::runtime_error &e) {
			done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_path, e.what()));
			return;
		}
	}

	genie::assets.blobs.clear();

	for (unsigned i = 0; i < COUNT; ++i) {
		try {
			genie::assets.blobs.emplace_back(new genie::DRS(fnames[i], fd[i], i != 3));
		} catch (std::runtime_error &e) {
			for (unsigned j = i + 1; j < COUNT; ++j)
				genie::close(fd[j]);

			genie::assets.blobs.clear();
			done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_path, e.what()));
			return;
		}
	}

	++p.step;
	set_desc("Verifying data resource sets");

	for (unsigned i = 0; i < res.size(); ++i) {
		if (genie::assets.blobs[i]->empty()) {
			done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_drs_empty, fnames[i].c_str()));
			return;
		}
		genie::DrsItem dummy;

		const std::vector<ResChk> &l = res[i];
		for (auto &r : l)
			if (!genie::assets.blobs[i]->open_item(dummy, r.id, (genie::DrsType)r.type)) {
				done(WorkTaskType::check_root, WorkResultType::fail, TXTF(TextId::err_no_drs_item, (unsigned)r.id));
				return;
			}
	}

	++p.step;
	set_desc("Looking for game fonts");

	genie::assets.ttf.clear();

#if WIN32
	std::string base("C:\\Windows\\Fonts\\");
	size_t off = 0;
#else
	std::string base(dir + "/../system/fonts/");
	size_t off = dir.size() + 1 + 2 + 1;
#endif

	for (unsigned i = 0; i < IM_ARRAYSIZE(ttfnames); ++i) {
		try {
			std::string path(base + ttfnames[i]);
			genie::assets.ttf.emplace_back(new genie::Blob(path, genie::open(path, off), true));
		} catch (std::runtime_error &e) {
			done(WorkTaskType::check_root, WorkResultType::fail, e.what());
			return;
		}
	}

	++p.step;
	genie::game_dir = dir;
	done(WorkTaskType::check_root, WorkResultType::success, TXT(TextId::work_drs_success));
#undef COUNT
}

}

static void worker_init(genie::Worker &w, int &status) {
	status = w.run();
}

enum class BinType {
	dialog,
	bitmap,
	palette,
	blob,
	text,
	shapelist,
};

static const char *bin_types[] = {
	"screen menu/dialog",
	"bitmap",
	"color palette",
	"binary data",
	"text",
	"shape list",
};

// TODO maak dit generieker voor gfx subsystem -> dat wordt texture & texturebuilder dus
class Preview final {
public:
	GLuint tex;
	SDL_Rect bnds; // x,y are hotspot x and y. w,h are size
	int offx, offy;
	std::string alt;

	Preview() : tex(0), bnds(), offx(0), offy(0), alt() {
		glGenTextures(1, &tex);
		if (!tex)
			throw std::runtime_error("Cannot create preview texture");
	}

	Preview(const Preview&) = delete;

	~Preview() {
		glDeleteTextures(1, &tex);
	}

	void load(const genie::Image &img, const SDL_Palette *pal) {
		genie::SubImagePreview preview(img, pal);
		bnds = preview.bnds;
		alt = "";

		GLuint old_tex;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&old_tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_UNPACK_ROW_LENGTH
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bnds.w, bnds.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, preview.pixels.data());
		glBindTexture(GL_TEXTURE_2D, old_tex);
	}

	void show() {
		if (!bnds.w || !bnds.h || !alt.empty()) {
			ImGui::TextUnformatted(alt.empty() ? "(no image data)" : alt.c_str());
		} else {
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offx);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offy);
			ImGui::Image((ImTextureID)tex, ImVec2((float)bnds.w, (float)bnds.h));
		}
	}
};

enum class UnitType {
	chariot_archer,
	archer,
	improved_archer,
	axeman,
	swordsman,
	ballista,
	stone_thrower,
	horse_archer,
	cataphract,
	max,
};

enum class UnitState {
	dead,
	die,
	idle,
	moving,
	attack,
	max,
};

static const char *unit_states_lst[] = {
	"decomposing", "dying", "waiting", "moving", "attacking",
};

class UnitInfo final {
public:
	unsigned attack_id, dead_id, decay_id, idle_id, moving_id;
	std::string name;

	UnitInfo(unsigned atk, unsigned die, unsigned decay, unsigned idle, unsigned moving, const std::string &name)
		: attack_id(atk), dead_id(die), decay_id(decay), idle_id(idle), moving_id(moving), name(name) {}

	UnitInfo(unsigned idle, const std::string &name)
		: attack_id(idle), dead_id(idle), decay_id(idle), idle_id(idle), moving_id(idle), name(name) {}
};

const std::map<std::string, UnitInfo> unit_info{
	{"missingno", {226, 342, 345, 444, 686, "pharaoh jason"}},
	{"archer1", {203, 308, 367, 413, 652, "bowman"}},
	{"archer2", {204, 309, 368, 414, 653, "improved bowman"}},
	{"archer3", {223, 337, 398, 439, 681, "composite bowman"}},
	{"cavarcher1", {202, 310, 369, 412, 650, "chariot archer"}},
	{"cavarcher2", {209, 318, 377, 422, 661, "horse archer"}},
	{"cavarcher3", {214, 323, 385, 427, 666, "elephant archer"}},
	{"infantry1", {212, 321, 380, 425, 664, "clubman"}},
	{"infantry2", {205, 311, 370, 415, 654, "axeman"}},
	{"infantry3", {206, 312, 371, 416, 655, "swordsman"}},
	{"infantry4", {220, 334, 395, 437, 678, "broad swordsman"}},
	{"infantry5", {219, 333, 394, 436, 677, "long swordsman"}},
	{"infantry6", {225, 340, 401, 442, 684, "hoplite"}},
	{"infantry7", {221, 335, 396, 438, 679, "phalanx"}},
	{"siege1", {629, 317, 376, 421, 660, "stone thrower"}},
	{"siege2", {208, 316, 375, 420, 659, "catapult"}},
	{"siege3", {207, 313, 372, 417, 656, "ballista"}},
	{"cavalry1", {227, 343, 403, 445, 651, "scout"}},
	{"cavalry2", {211, 320, 379, 424, 663, "cavalry"}}, // decomposing is bogus
	{"cavalry3", {210, 319, 378, 423, 662, "cataphract"}},
	{"cavalry4", {213, 322, 381, 426, 665, "chariot"}},
	{"elephant1", {216, 325, 387, 429, 669, "elephant"}},
	{"elephant2", {228, 446, 446, 446, 687, "tamed elephant"}},
	{"priest", {634, 341, 402, 443, 685, "priest"}},
	{"worker1", {224, 314, 373, 418, 657, "villager"}},
	{"worker2", {473, 473, 473, 473, 473, "fishing boat"}},
	{"worker3", {474, 474, 474, 474, 474, "fishing ship"}},
	{"worker4", {639, 639, 639, 639, 639, "merchant boat"}},
	{"worker5", {640, 640, 640, 640, 640, "merchant ship"}},
	{"artifact2", {244, "artifact"}}, // player owned
	{"cheat1", {604, 606, 605, 607, 603, "storm trooper"}},
	{"cheat2", {345, 345, 345, 715, 713, "storm trooper"}},
	// gaia
	{"elephant", {215, 324, 386, 428, 667, "elephant"}},
	{"gaia1", {217, 330, 391, 433, 673, "alligator"}}, // 477 swimming
	{"gaia2", {222, 336, 397, 497, 528, "lion"}}, // 680 moving1
	{"cow", {434, 345, 345, 434, 674, "cow"}},
	{"horse", {475, 345, 345, 475, 475, "horse"}},
	{"deer", {331, 331, 392, 479, 480, "deer"}}, // 478 moving1
	{"artifact", {245, "artifact"}},
	{"bird1", {406, 345, 345, 406, 404, "eagle"}},
	{"bird2", {521, 345, 345, 521, 518, "bird"}},
};

std::vector<const char*> unit_info_lst;

class UnitView final {
	int type;
	std::optional<genie::Animation> anim;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal;
	Preview preview;
	unsigned player;
	int offx, offy, subimage, state;
	bool changed, flip;
	const UnitInfo *info;
public:
	UnitView()
		: type(0), anim()
		, pal(genie::assets.blobs[2]->open_pal((unsigned)genie::DrsId::palette), SDL_FreePalette)
		, preview(), player(0), offx(0), offy(0), subimage(0), state(0), changed(true), flip(false), info(false)
	{
		if (unit_info_lst.empty()) {
			for (auto &kv : unit_info) {
				unit_info_lst.emplace_back(kv.first.c_str());
			}
		}
	}

	void compute_offset() {
		offx = offy = 0;

		for (unsigned i = 0, n = anim->image_count; i < n; ++i) {
			const genie::Image &img = anim->subimage(i, player);
			offx = std::max(offx, img.bnds.x);
			offy = std::max(offy, img.bnds.y);
		}
	}

	void set_anim(const UnitInfo &info, UnitState state) {
		genie::res_id id = -1;

		switch (state) {
			case UnitState::attack: id = info.attack_id; break;
			case UnitState::dead: id = info.decay_id; break;
			case UnitState::die: id = info.dead_id; break;
			case UnitState::idle: id = info.idle_id; break;
			case UnitState::moving: id = info.moving_id; break;
			default:
				throw std::runtime_error("bad unit state");
		}

		anim = std::move(genie::assets.blobs[1]->open_anim(id));
		compute_offset();
		this->info = &info;
		changed = true;
	}

	void show() {
		if (ImGui::Begin("Unit viewer")) {
			int rel_offx, rel_offy;

			int old_type = type;
			changed |= ImGui::Combo("type", &type, unit_info_lst.data(), unit_info_lst.size());

			if (changed && type >= 0 && type < unit_info_lst.size()) {
				std::string key(unit_info_lst[type]);
				auto it = unit_info.find(key);
				set_anim(it->second, (UnitState)std::clamp(state, 0, (int)UnitState::max - 1));
			}

			if (!anim.has_value()) {
				ImGui::TextUnformatted("no type selected");
			} else {
				changed |= ImGui::Combo("action", &state, unit_states_lst, IM_ARRAYSIZE(unit_states_lst));
				if (changed && state >= 0 && state < IM_ARRAYSIZE(unit_states_lst)) {
					std::string key(unit_info_lst[type]);
					auto it = unit_info.find(key);
					set_anim(it->second, (UnitState)state);
				}

				ImGui::Text("name: %s", info->name.c_str());
				ImGui::Text("max offx: %d, max offy: %d", offx, offy);
				changed |= ImGui::Checkbox("flip", &flip);
				changed |= ImGui::SliderInputInt("subimage", &subimage, -anim->image_count + 1, 2 * anim->image_count);
				if (subimage < 0)
					subimage += anim->image_count;
				if (subimage >= anim->image_count)
					subimage %= anim->image_count;

				if (changed) {
					const genie::Image &img = anim->subimage(subimage, player);

					if (flip) {
						genie::Image imgf(std::move(img.flip()));

						preview.load(imgf, pal.get());

						rel_offx = offx - imgf.bnds.x;
						rel_offy = offy - imgf.bnds.y;
					} else {
						preview.load(img, pal.get());

						rel_offx = offx - img.bnds.x;
						rel_offy = offy - img.bnds.y;
					}

					preview.offx = rel_offx;
					preview.offy = rel_offy;
				}

				preview.show();
			}

			changed = false;
		}
		ImGui::End();
	}
};

class DrsView final {
public:
	int current_drs, current_list, current_item, current_image, current_player, current_palette, channel, dialog_mode, lookup_id;
	bool looping, autoplay, preview_changed;
	genie::io::DrsItem item;
	std::variant<std::nullopt_t, BinType, genie::Dialog, SDL_Palette*, genie::Animation, genie::io::Slp> resdata;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal;
	genie::DrsItem res;
	Preview preview;

	DrsView()
		: current_drs(-1), current_list(-1), current_item(-1), current_image(-1), current_player(-1), current_palette(-1)
		, channel(-1), dialog_mode(0), lookup_id(0), looping(false), autoplay(true), preview_changed(true)
		, item(), resdata(std::nullopt), pal(nullptr, SDL_FreePalette), res(), preview() {}

	DrsView(const DrsView&) = delete;
	~DrsView() { reset(); }

	void reset() {
		res.data = nullptr;

		switch (resdata.index()) {
			case 3: SDL_FreePalette(std::get<SDL_Palette*>(resdata)); break;
		}

		resdata.emplace<std::nullopt_t>(std::nullopt);
	}

	void show() {
#pragma warning (push)
#pragma warning (disable: 4267)
		// setup stuff, this cannot be done in the ctor as genie::assets.blobs are undefined in the ctor
		if (current_palette == -1) {
			pal.reset(genie::assets.blobs[2]->open_pal((unsigned)genie::DrsId::palette));
			current_palette = (int)genie::DrsId::palette;
		}

		int old_drs = current_drs, old_list = current_list, old_item = current_item;
		bool found = false;

		ImGui::InputInt("##lookup", &lookup_id);
		ImGui::SameLine();
		if (ImGui::Button("lookup")) {

			for (unsigned i = 0; !found && i < genie::assets.blobs.size(); ++i) {
				const genie::DRS &drs = *genie::assets.blobs[i].get();

				for (unsigned j = 0; !found && j < drs.size(); ++j) {
					genie::DrsList lst(drs[j]);

					for (unsigned k = 0; k < lst.size; ++k)
						if (lst[k].id == lookup_id) {
							found = true;
							current_drs = i;
							current_list = j;
							current_item = k;
							break;
						}
				}
			}
		}

		float start = ImGui::GetCursorPosY();
		ImGui::SliderInputInt("container", &current_drs, 0, genie::assets.blobs.size() - 1);
		float h = ImGui::GetCursorPosY() - start;
		current_drs = genie::clamp<int>(0, current_drs, genie::assets.blobs.size() - 1);

		const genie::DRS &drs = *genie::assets.blobs[current_drs].get();

		if (drs.size() > 1) {
			size_t max = drs.size() - 1;
			ImGui::SliderInputInt("list", &current_list, 0, max);
			current_list = genie::clamp<int>(0, current_list, max);
		} else {
			current_list = 0;
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + h);
		}

		genie::DrsList lst(drs[current_list]);
		genie::DrsType drs_type = genie::DrsType::binary;
		const char *str_drs_type = "???";

		switch (lst.type) {
			case genie::drs_type_bin: drs_type = genie::DrsType::binary; str_drs_type = "various"   ; break;
			case genie::drs_type_shp: drs_type = genie::DrsType::shape ; str_drs_type = "shape"     ; break;
			case genie::drs_type_slp: drs_type = genie::DrsType::slp   ; str_drs_type = "shape list"; break;
			case genie::drs_type_wav: drs_type = genie::DrsType::wave  ; str_drs_type = "audio"     ; break;
		}

		ImGui::Text("Type  : %" PRIX32 " %s\nOffset: %" PRIX32 "\nItems : %" PRIu32, lst.type, str_drs_type, lst.offset, lst.size);

		ImGui::InputInt("item", &current_item);
		current_item = genie::clamp<int>(0, current_item, lst.size - 1);

		item = lst[current_item];

		ImGui::Text("id    : %" PRIu32 "\noffset: %" PRIX32 "\nsize  : %" PRIX32, item.id, item.offset, item.size);

		if (old_drs != current_drs || old_list != current_list || old_item != current_item || found) {
			preview_changed = true;

			if (channel != -1) {
				genie::jukebox.stop(channel);
				channel = -1;
			}

			reset();
			bool loaded = lst.type != genie::drs_type_wav && drs.open_item(res, item.id, drs_type);

			switch (lst.type) {
				case genie::drs_type_bin: {
					if (res.size < 8) {
						resdata.emplace<BinType>(BinType::blob);
						break;
					}
					const char *str = (const char*)res.data;

					if (str[0] == 'B' && str[1] == 'M') {
						resdata.emplace<BinType>(BinType::bitmap);
						break;
					}
					if (strncmp(str, "JASC-PAL", 8) == 0) {
						try {
							resdata.emplace<SDL_Palette*>(drs.open_pal(item.id));
						} catch (std::runtime_error&) {
							resdata.emplace<BinType>(BinType::palette);
						}
						break;
					}
					if (strncmp(str, "backgrou", 8) == 0) {
						try {
							resdata.emplace<genie::Dialog>(drs.open_dlg(item.id));
						} catch (std::runtime_error&) {
							resdata.emplace<BinType>(BinType::dialog);
						}
						break;
					}
					bool looks_binary = false;

					for (unsigned i = 0; i < 8; ++i)
						if (!isprint((unsigned char)str[i])) {
							looks_binary = true;
							break;
						}

					resdata.emplace<BinType>(looks_binary ? BinType::blob : BinType::text);
					break;
				}
				case genie::drs_type_slp:
					try {
						resdata.emplace<genie::Animation>(drs.open_anim(item.id));
					} catch (std::runtime_error&) {
						try {
							resdata.emplace<genie::io::Slp>(drs.open_slp(item.id));
						} catch (std::runtime_error&) {
							resdata.emplace<BinType>(BinType::shapelist);
						}
					}
					break;
				case genie::drs_type_wav:
					if (autoplay)
						channel = genie::jukebox.sfx((genie::DrsId)item.id, looping ? -1 : 0);
					break;
			}
		}

		switch (lst.type) {
			case genie::drs_type_bin:
				switch (resdata.index()) {
					case 1: { // BinType
						BinType type = std::get<BinType>(resdata);

						ImGui::Text("Type: %s", bin_types[(unsigned)type]);

						switch (type) {
							case BinType::blob:
								ImGui::TextUnformatted("unknown binary blob");
								break;
							case BinType::dialog:
							case BinType::palette:
							case BinType::text:
								ImGui::TextWrapped("%.*s", (int)res.size, (const char*)res.data);
								break;
							case BinType::bitmap:
								ImGui::TextUnformatted("bitmap");
								break;
							case BinType::shapelist:
								ImGui::TextUnformatted("unknown shape list");
								break;
						}
						break;
					}
					case 2: { // Dialog
						ImGui::Text("Type: %s", bin_types[(unsigned)BinType::dialog]);

						if (ImGui::TreeNode("Raw")) {
							ImGui::TextWrapped("%.*s", (int)res.size, (const char*)res.data);
							ImGui::TreePop();
						}

						genie::Dialog &dlg = std::get<genie::Dialog>(resdata);

						if (ImGui::TreeNode("Preview")) {
							int old_dialog_mode = dialog_mode;
							ImGui::Combo("Display mode", &dialog_mode, str_dim_lgy, IM_ARRAYSIZE(dim_lgy));

							if (old_dialog_mode != dialog_mode)
								preview_changed = true;

							if (preview_changed) {
								try {
									dlg.set_bkg(dialog_mode);
									auto &anim = *dlg.bkganim.get();
									preview.load(anim.subimage(0), dlg.pal.get());
								} catch (std::runtime_error&) {
									preview.alt = "(invalid image data)";
								}
								preview_changed = false;
							}

							preview.show();
							ImGui::TreePop();
						}

						auto &cfg = dlg.cfg;

						if (ImGui::TreeNode("Palette")) {
							ImGui::Text("ID: %" PRIu16, cfg.pal);
							ImGui::ColorPalette(dlg.pal.get());
							ImGui::TreePop();
						}

						ImGui::Text("Cursor : %" PRIu16 "\nShade  : %d\nButton : %" PRIu16 "\nPopup  : %" PRIu16, cfg.cursor, cfg.shade, cfg.btn, cfg.popup);
						break;
					}
					case 3: { // SDL_Palette*
						ImGui::Text("Type: %s", bin_types[(unsigned)BinType::palette]);

						SDL_Palette *pal = std::get<SDL_Palette*>(resdata);
						ImGui::ColorPalette(pal);
						break;
					}
				}
				break;
			case genie::drs_type_slp:
				switch (resdata.index()) {
					case 4: { // genie::Animation
						genie::Animation &anim = std::get<genie::Animation>(resdata);
						ImGui::Text("Type  : animation\nImages: %u\nDynamic: %s", anim.image_count, anim.dynamic ? "yes" : "no");

						int old_image = current_image, old_player = current_player;

						if (anim.image_count > 1) {
							ImGui::SliderInputInt("image", &current_image, 0, anim.image_count - 1);
							current_image = genie::clamp<int>(0, current_image, anim.image_count - 1);
						}

						if (anim.dynamic) {
							ImGui::SliderInputInt("player", &current_player, 0, genie::io::max_players - 1);
							current_player = genie::clamp<int>(0, current_player, genie::io::max_players - 1);
						} else {
							current_player = 0;
						}

						if (old_image != current_image || old_player != current_player)
							preview_changed = true;

						if (anim.image_count) {
							auto &img = anim.subimage(current_image, current_player);

							ImGui::Text("Size: %dx%d\nCenter: %d,%d", img.bnds.w, img.bnds.h, img.bnds.x, img.bnds.y);

							if (anim.image_count && ImGui::TreeNode("Preview")) {
								int old_palette = current_palette;
								ImGui::InputInt("Palette", &current_palette);
								if (old_palette != current_palette) {
									try {
										pal.reset(genie::assets.open_pal(current_palette));
										preview_changed = true;
									} catch (std::runtime_error&) {
										current_palette = old_palette;
									}
								}

								if (preview_changed) {
									preview.load(anim.subimage(current_image, current_player), pal.get());
									preview_changed = false;
								}

								preview.show();
								ImGui::TreePop();
							}
						}
						break;
					}
					case 5: { // genie::io::Slp
						genie::io::Slp &slp = std::get<genie::io::Slp>(resdata);
						ImGui::Text("Type  : shape list\nSize  : %llu\nFrames: %" PRId32 "\ninvalid frame data", (unsigned long long)slp.size, slp.hdr->frame_count);
						// don't show data as it's corrupt anyway
						break;
					}
				}
				break;
			case genie::drs_type_wav:
				if (channel != -1 && !Mix_Playing(channel))
					channel = -1;

				if (ImGui::Button(channel != -1 ? "stop" : "play"))
					if (channel == -1) {
						channel = genie::jukebox.sfx((genie::DrsId)item.id, looping ? -1 : 0);
					} else {
						genie::jukebox.stop(channel);
						channel = -1;
					}
				ImGui::SameLine();
				ImGui::Checkbox("loop", &looping);
				ImGui::SameLine();
				ImGui::Checkbox("autoplay", &autoplay);
				break;
		}
#pragma warning (pop)
	}
};

class GlobalConfiguration;

static constexpr bool contains(double left, double top, double right, double bottom, double x, double y) {
	return x >= left && y >= top && x < right && y < bottom;
}
namespace genie {

VideoControl::VideoControl() : displays(SDL_GetNumVideoDisplays()), display(0), mode(3), aspect(true), legacy(true), fullscreen(false), bounds(displays > 0 ? displays : 0), size(), winsize({0, 0, 800, 600}), vp() {
	for (int i = 0; i < displays; ++i)
		SDL_GetDisplayBounds(i, &bounds[i]);
}

void VideoControl::motion(double &px, double &py, int x, int y) {
	if (x < vp.x || y < vp.y || x >= vp.x + vp.w || y >= vp.y + vp.h) {
		px = py = -1;
		return;
	}

	if (vp.w == size.w && vp.h == size.h) {
		px = x; py = y;
	} else {
		px = (x - vp.x) * (double)(size.w - 2 * vp.x) / vp.w;
		py = (y	- vp.y) * (double)(size.h - 2 * vp.y) / vp.h;
	}
}

void VideoControl::idle() {
	if (fullscreen)
		return; // Disable mode determination

	// Default to `Custom' mode
	mode = 3;

	for (int i = 0; i < IM_ARRAYSIZE(dim_lgy); ++i)
		if (size.w == dim_lgy[i].w && size.h == dim_lgy[i].h) {
			mode = i;
			break;
		}

	if (!fullscreen) {
		SDL_GetWindowSize(window, &winsize.w, &winsize.h);
		SDL_GetWindowPosition(window, &winsize.x, &winsize.y);
	}
}

void VideoControl::center() {
	if (fullscreen)
		return;

	int index = SDL_GetWindowDisplayIndex(window);

	if (index < 0)
		index = 0;

	int left, top;
	const SDL_Rect &d = bounds[index];

	left = d.x + (d.w - winsize.w) / 2;
	top = d.y + (d.h - winsize.h) / 2;

	SDL_SetWindowPosition(window, left, top);
}

void VideoControl::set_fullscreen(bool enable) {
	SDL_Rect bnds;
	bool was_windowed = !(SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP));
	int index = SDL_GetWindowDisplayIndex(window);

	if (index < 0)
		index = 0;

	SDL_GetWindowSize(window, &bnds.w, &bnds.h);

	if (!enable) {
		if (!was_windowed) {
			SDL_SetWindowFullscreen(window, 0);
			SDL_SetWindowPosition(window, winsize.x, winsize.y);
		}

		SDL_SetWindowSize(window, winsize.w, winsize.h);
	} else if (was_windowed) {
		SDL_SetWindowPosition(window, bounds[index].x, bounds[index].y);
		SDL_SetWindowSize(window, bounds[index].w, bounds[index].h);

		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}

	fullscreen = enable;
}

static constexpr const char *defcfg = "settings.dat";
static constexpr uint32_t cfghdr = 0x1b28d7a4; // Nothing special, just smashing some keys

void GlobalConfiguration::load(const VideoControl &video) {
	display_index = video.display;
	mode = video.mode;
	display = video.bounds[display_index];
	fullscreen = (SDL_GetWindowFlags(window) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;

	if (scrbnds.w < 1 || scrbnds.h < 1)
		scrbnds = dim_lgy[2];

	if (!fullscreen) {
		SDL_GetWindowPosition(window, &scrbnds.x, &scrbnds.y);
		SDL_GetWindowSize(window, &scrbnds.w, &scrbnds.h);
	}
}

bool GlobalConfiguration::load() { return load(defcfg); }

bool GlobalConfiguration::load(const char *fname) {
	try {
		std::ifstream in(fname, std::ios_base::binary);
		in.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		uint8_t db; uint32_t dd;
		int8_t b; int16_t w; int32_t d;

		SDL_Rect display, scrbnds;
		int display_index, mode;
		bool fullscreen, msc_on, sfx_on;
		int msc_vol, sfx_vol;

		in.read((char*)&dd, 4);
		if (dd != cfghdr)
			throw std::runtime_error("bad magic header");

		in.read((char*)&b, 1); display_index = b;
		in.read((char*)&b, 1); mode = b;

		in.read((char*)&db, 1);
		fullscreen = (db & 0x01) != 0;
		msc_on = (db & 0x02) != 0;
		sfx_on = (db & 0x04) != 0;

		in.read((char*)&b, 1);

		in.read((char*)&d, 4); display.x = d;
		in.read((char*)&d, 4); display.y = d;
		in.read((char*)&d, 4); display.w = d;
		in.read((char*)&d, 4); display.h = d;

		if (display.w < 1 || display.h < 1)
			throw std::runtime_error("invalid display size");

		in.read((char*)&d, 4); scrbnds.x = d;
		in.read((char*)&d, 4); scrbnds.y = d;
		in.read((char*)&d, 4); scrbnds.w = d;
		in.read((char*)&d, 4); scrbnds.h = d;

		if (scrbnds.w < 1 || scrbnds.h < 1)
			throw std::runtime_error("invalid application size");

		in.read((char*)&w, 2); msc_vol = w;
		in.read((char*)&w, 2); sfx_vol = w;

		std::string gamepath;

		in.read((char*)&w, 2); gamepath.resize(w, '\0');
		in.read((char*)gamepath.data(), w);

		this->display = display;
		this->scrbnds = scrbnds;
		this->display_index = display_index;
		this->mode = mode;
		this->fullscreen = fullscreen;
		this->gamepath = gamepath;

		genie::jukebox.msc(msc_on); genie::jukebox.msc_volume(msc_vol);
		genie::jukebox.sfx(sfx_on); genie::jukebox.sfx_volume(sfx_vol);

		return true;
	} catch (std::ios_base::failure &f) {
		fprintf(stderr, "%s: %s\n", fname, f.what());
	} catch (std::runtime_error &e) {
		fprintf(stderr, "%s: %s\n", fname, e.what());
	}

	return false;
}

void GlobalConfiguration::save() { save(defcfg); }

void GlobalConfiguration::save(const char *fname) {
	try {
		std::ofstream out(fname, std::ios_base::binary);
		out.exceptions(std::ofstream::failbit | std::ofstream::badbit);

		uint8_t db;
		int8_t b; int16_t w; int32_t d;

		out.write((char*)&cfghdr, 4);

		b = display_index; out.write((char*)&b, 1);
		b = mode; out.write((char*)&b, 1);

		db = 0;
		if (fullscreen)
			db |= 0x01;
		if (genie::jukebox.msc_enabled())
			db |= 0x02;
		if (genie::jukebox.sfx_enabled())
			db |= 0x04;
		out.write((char*)&db, 1);

		b = 0; out.write((char*)&b, 1);

		d = display.x; out.write((char*)&d, 4);
		d = display.y; out.write((char*)&d, 4);
		d = display.w; out.write((char*)&d, 4);
		d = display.h; out.write((char*)&d, 4);

		d = scrbnds.x; out.write((char*)&d, 4);
		d = scrbnds.y; out.write((char*)&d, 4);
		d = scrbnds.w; out.write((char*)&d, 4);
		d = scrbnds.h; out.write((char*)&d, 4);

		w = genie::jukebox.msc_volume(); out.write((char*)&w, 2);
		w = genie::jukebox.sfx_volume(); out.write((char*)&w, 2);

		w = gamepath.length(); out.write((char*)&w, 2);
		if (w)
			out.write((char*)gamepath.data(), w);
	} catch (std::ios_base::failure &f) {
		fprintf(stderr, "%s: %s\n", fname, f.what());
	}
}

GlobalConfiguration settings;

void VideoControl::restore(GlobalConfiguration &cfg) {
	// only apply configuration if display settings match
	bool match = false;

	for (SDL_Rect b : bounds)
		if (b.x == cfg.display.x && b.y == cfg.display.y && b.w == cfg.display.w && b.h == cfg.display.h) {
			match = true;
			break;
		}

	SDL_Rect cntr = cfg.scrbnds;
	cntr.x += cfg.scrbnds.w / 2;
	cntr.y += cfg.scrbnds.h / 2;

	if (!match || SDL_GetNumVideoDisplays() < cfg.display_index
		// Also don't restore if window is out of bounds
		|| cfg.scrbnds.x + cfg.scrbnds.w < cfg.display.x || cfg.scrbnds.y + cfg.scrbnds.h < cfg.display.y
		|| cntr.x >= cfg.display.x + cfg.display.w || cntr.y >= cfg.display.y + cfg.display.h)
		return;

	winsize = cfg.scrbnds;
	SDL_SetWindowPosition(window, cfg.scrbnds.x, cfg.scrbnds.y);
	SDL_SetWindowSize(window, cfg.scrbnds.w, cfg.scrbnds.h);

	set_fullscreen(cfg.fullscreen);
}

class Hud;

}

static ImFont *fonts[6];
static int font_sizes[6] = {14, 14, 16, 16, 18, 32};


IMGUI_API int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);

static const unsigned anim_count[] = {
	25,
	25,
	20,
	20,
	36,
	36,
	68,
};

namespace genie {

void Hud::reset() {
	mouse_btn = 0;
	mouse_btn_grab = 0;
	ui_popup = nullptr;
	items.clear();
	keep.clear();
}

void Hud::reset(genie::Texture &tex) {
	this->tex = &tex;
	reset();
}

void Hud::down(SDL_MouseButtonEvent &ev) {
	Uint8 db = ev.button;
	mouse_btn |= SDL_BUTTON(db);
	mouse_btn_grab &= ~SDL_BUTTON(db);
}

void Hud::up(SDL_MouseButtonEvent &ev) {
	Uint8 db = ev.button;
	mouse_btn &= ~SDL_BUTTON(db);
	mouse_btn_grab &= ~SDL_BUTTON(db);

	if (ui_popup) {
		// determine selected element
		for (unsigned i = 0; i < ui_popup->item_bounds.size(); ++i) {
			SDL_Rect &bnds = ui_popup->item_bounds[i];

			if (mouse_x >= bnds.x && mouse_y >= bnds.y && mouse_x < bnds.x + bnds.w && mouse_y < bnds.y + bnds.h) {
				ui_popup->item_select(i);
				break;
			}
		}

		ui_popup->opened = false;
		ui_popup->closed = true;
		ui_popup = nullptr;
	}
}

bool Hud::try_grab(Uint8 mask, bool hot) {
	if (ui_popup)
		return false;

	if (hot && (mouse_btn & mask) != 0) {
		if (mouse_btn_grab & mask)
			return false;

		mouse_btn_grab |= mask;
		return true;
	}

	return false;
}

bool Hud::lost_grab(Uint8 mask, bool active) {
	return active && (mouse_btn & mask) == 0;
}

void Hud::display() {
	for (unsigned id : keep)
		items.find(id)->second->display(*this);

#if 0
	for (auto it = items.begin(); it != items.end();) {
		if (keep.find(it->first) == keep.end())
			it = items.erase(it);
		else
			++it;
	}
#endif
	// reset state
	keep.clear();

	if (ui_popup)
		ui_popup->display_popup(*this);
}

bool Hud::button(unsigned id, unsigned fnt_id, double x, double y, double w, double h, const std::string &label, int shade, SDL_Color cols[6]) {
	// copy state
	left = x; top = y; right = x + w; bottom = y + h;
	m_text = label;
	this->fnt_id = fnt_id;
	this->shade = shade;

	for (unsigned i = 0; i < 6; ++i)
		this->cols[i] = cols[i];

	// register component
	keep.emplace(id);

	// create/update element
	auto search = items.find(id);

	bool b = false;

	if (search == items.end()) {
		auto btn = items.emplace(id, new Button(id)).first;
		b = btn->second->update(*this);
		if (btn->second->hot)
			was_hot = true;
	} else {
		b = search->second->update(*this);
		if (search->second->hot)
			was_hot = true;
	}

	return b;
}

bool Hud::listpopup(unsigned id, unsigned fnt_id, double x, double y, double w, double h, unsigned &selected, const std::vector<std::string> &elements, int shade, SDL_Color cols[6], bool left) {
	// copy state
	this->left = x; top = y; right = x + w; bottom = y + h;
	this->fnt_id = fnt_id;
	this->shade = shade;
	this->align_left = left;

	for (unsigned i = 0; i < 6; ++i)
		this->cols[i] = cols[i];

	// register component
	keep.emplace(id);

	// create/update element
	auto search = items.find(id);

	bool b = false;
	ListPopup *ui = nullptr;

	if (search == items.end()) {
		auto popup = items.emplace(id, new ListPopup(id, fnt_id, selected, elements)).first;
		ui = (ListPopup*)popup->second.get();
	} else {
		ui = (ListPopup*)search->second.get();
	}

	if (ui) {
		if (ui->closed) {
			ui->closed = false;
			selected = ui->selected_item;
			b |= true;
		} else {
			ui->selected_item = selected;
		}

		b |= ui->update(*this);

		if (ui->hot) {
			was_hot = true;

			if (b) {
				ui->opened = true;
				ui_popup = ui;
			}
		}
	}

	return b;
}

void Hud::rect(SDL_Rect bnds, SDL_Color col[6]) {
	glBegin(GL_QUADS);
	{
		GLdouble left = bnds.x, top = bnds.y, right = left + bnds.w, bottom = top + bnds.h;
		// start with outermost layers
		glColor4ub(col[0].r, col[0].g, col[0].b, 255);
		glVertex2d(left, top); glVertex2d(right, top); glVertex2d(right, top + 1.); glVertex2d(left, top + 1.);
		glVertex2d(right - 1., top); glVertex2d(right, top); glVertex2d(right, bottom); glVertex2d(right - 1., bottom);

		glColor4ub(col[1].r, col[1].g, col[1].b, 255);
		glVertex2d(left + 1., top + 1.); glVertex2d(right - 1., top + 1.); glVertex2d(right - 1, top + 2); glVertex2i(left + 1., top + 2.);
		glVertex2d(right - 2., top + 2.); glVertex2d(right - 1., top + 2.); glVertex2d(right - 1, bottom); glVertex2i(right - 2., bottom);

		glColor4ub(col[2].r, col[2].g, col[2].b, 255);
		glVertex2d(left + 2., top + 2.); glVertex2d(right - 2., top + 2.); glVertex2d(right - 2., top + 2.4); glVertex2d(left + 2., top + 2.4);
		glVertex2d(right - 3., top + 2.); glVertex2d(right - 2., top + 2.); glVertex2d(right - 2., bottom); glVertex2d(right - 3., bottom);

		// continue with outermost and use diagonal lines to ensure the overlapping area closes any gaps
		glColor4ub(col[5].r, col[5].g, col[5].b, 255);
		glVertex2d(left, top); glVertex2d(left + 1., top); glVertex2d(left + 1., bottom); glVertex2d(left, bottom);
		glVertex2d(left, bottom - 1.); glVertex2d(right, bottom - 1.); glVertex2d(right, bottom); glVertex2d(left, bottom);

		glColor4ub(col[4].r, col[4].g, col[4].b, 255);
		glVertex2d(left + 1., top + 1.); glVertex2d(left + 2., top + 1.); glVertex2d(left + 2., bottom - 1.); glVertex2d(left + 1., bottom - 1.);
		glVertex2d(left + 2., bottom - 2.); glVertex2d(right - 1., bottom - 2.); glVertex2d(right - 1., bottom - 1.); glVertex2d(left + 2., bottom - 1.);

		glColor4ub(col[3].r, col[3].g, col[3].b, 255);
		glVertex2d(left + 2., top + 2.); glVertex2d(left + 2.4, top + 2.); glVertex2d(left + 2.4, bottom - 2.); glVertex2d(left + 2., bottom - 2.);
		glVertex2d(left + 2., bottom - 3.); glVertex2d(right - 2., bottom - 3.); glVertex2d(right - 2., bottom - 2.); glVertex2d(left + 2., bottom - 2.);
	}
	glEnd();
}

void Hud::shaderect(SDL_Rect bnds, int shade) {
	if (shade >= 255)
		return;

	glBegin(GL_QUADS);
	{
		glColor4ub(0, 0, 0, shade);
		glVertex2i(bnds.x, bnds.y); glVertex2i(bnds.x + bnds.w, bnds.y);
		glVertex2i(bnds.x + bnds.w, bnds.y + bnds.h); glVertex2i(bnds.x, bnds.y + bnds.h);
	}
	glEnd();
}

void Hud::text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const std::string &str, float width, int align)
{
	text(font, size, pos, col, clip_rect, str.c_str(), str.size(), width, align);
}

void Hud::text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const std::string &str, float width, int align, float &max_width)
{
	text(font, size, pos, col, clip_rect, str.c_str(), str.size(), width, align, max_width);
}

void Hud::text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align)
{
	float dummy = 0;
	text(font, size, pos, col, clip_rect, text_begin, n, wrap_width, align, dummy);
}

void Hud::text(ImFont *font, float size, ImVec2 &pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align, float &max_width)
{
#ifndef IM_FLOOR
#define IM_FLOOR(_VAL)                  ((float)(int)(_VAL))
#endif
	max_width = 0.0f;

	if (!n)
		return;

	const char *text_end = text_begin + n;

	pos.x = IM_FLOOR(pos.x + font->DisplayOffset.x);
	pos.y = IM_FLOOR(pos.y + font->DisplayOffset.y);
	float x = pos.x, x_orig = x;
	float y = pos.y;
	if (y > clip_rect.y + clip_rect.h)
		return;

	const float scale = size / font->FontSize;
	const float line_height = font->FontSize * scale;
	const bool word_wrap_enabled = wrap_width > 0.f;
	const char *word_wrap_eol = NULL;

	// Fast-forward to first visible line
	const char *s = text_begin;
	if (y + line_height < clip_rect.y && !word_wrap_enabled)
		while (y + line_height < clip_rect.y && s < text_end) {
			s = (const char*)memchr(s, '\n', text_end - s);
			s = s ? s + 1 : text_end;
			y += line_height;
		}

	glColor4ub(col.r, col.g, col.b, col.a);
	glBegin(GL_QUADS);

	// Align
	float adjust = 0;

	if (align != -1) {
		for (const char *s = text_begin; s < text_end;) {
			unsigned int c = (unsigned int)*s;
			if (c < 0x80) {
				++s;
			} else {
				s += ImTextCharFromUtf8(&c, s, text_end);
				if (!c)
					break;
			}

			if (c == '\r')
				continue;

			if (c == '\n')
				break;

			const ImFontGlyph *glyph = font->FindGlyph((ImWchar)c);
			if (!glyph)
				continue;

			adjust += glyph->AdvanceX * scale;
		}

		if (align == 0)
			adjust *= 0.5f;
	}

	for (const char *s = text_begin; s < text_end;) {
		if (word_wrap_enabled) {
			if (!word_wrap_eol) {
				word_wrap_eol = font->CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));

				if (word_wrap_eol == s)
					++word_wrap_eol;
			}

			if (s >= word_wrap_eol) {
				x = pos.x;
				y += line_height;
				word_wrap_eol = NULL;

				// Wrapping skips upcoming blanks
				while (s < text_end) {
					const char *c = s;
					if (*c == ' ' || *c == '\t') {
						++s;
					} else {
						if (*c == '\n')
							++s;
						break;
					}
				}
				continue;
			}
		}

		// Decode and advance source
		unsigned int c = (unsigned int)*s;
		if (c < 0x80) {
			++s;
		} else {
			s += ImTextCharFromUtf8(&c, s, text_end);
			if (!c)
				break;
		}

		if (c < 32) {
			if (c == '\n') {
				x = pos.x;
				y += line_height;
				if (y > clip_rect.y + clip_rect.h)
					break; // break out of main loop
				continue;
			}
			if (c == '\r')
				continue;
		}

		const ImFontGlyph *glyph = font->FindGlyph((ImWchar)c);
		if (!glyph)
			continue;

		float char_width = glyph->AdvanceX * scale;
		if (glyph->Visible) {
			// We don't do a second finer clipping test on the Y axis as we've already skipped anything before clip_rect.y and exit once we pass clip_rect.w
			float x1 = x + glyph->X0 * scale - adjust;
			float x2 = x + glyph->X1 * scale - adjust;
			float y1 = y + glyph->Y0 * scale;
			float y2 = y + glyph->Y1 * scale;

			if (x1 <= clip_rect.x + clip_rect.w && x2 >= clip_rect.x) {
				// Render a character
				float u1 = glyph->U0;
				float v1 = glyph->V0;
				float u2 = glyph->U1;
				float v2 = glyph->V1;

				glTexCoord2f(u1, v1); glVertex2f(x1, y1);
				glTexCoord2f(u2, v1); glVertex2f(x2, y1);
				glTexCoord2f(u2, v2); glVertex2f(x2, y2);
				glTexCoord2f(u1, v2); glVertex2f(x1, y2);
			}
		}
		x += char_width;

		if (x > max_width)
			max_width = x;
	}

	glEnd();

	pos.x = x;
	pos.y = y;
	max_width -= x_orig;
}

void Hud::textdim(ImFont *font, float size, ImVec2 &pos, const SDL_Rect &clip_rect, const std::string &str, float wrap_width, int align, float &max_width)
{
	textdim(font, size, pos, clip_rect, str.c_str(), str.size(), wrap_width, align, max_width);
}

void Hud::textdim(ImFont *font, float size, ImVec2 &pos, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align, float &max_width)
{
#ifndef IM_FLOOR
#define IM_FLOOR(_VAL)                  ((float)(int)(_VAL))
#endif
	max_width = 0.0f;

	if (!n)
		return;

	const char *text_end = text_begin + n;

	pos.x = IM_FLOOR(pos.x + font->DisplayOffset.x);
	pos.y = IM_FLOOR(pos.y + font->DisplayOffset.y);
	float x = pos.x, x_orig = x;
	float y = pos.y;
	if (y > clip_rect.y + clip_rect.h)
		return;

	const float scale = size / font->FontSize;
	const float line_height = font->FontSize * scale;
	const bool word_wrap_enabled = wrap_width > 0.f;
	const char *word_wrap_eol = NULL;

	// Fast-forward to first visible line
	const char *s = text_begin;
	if (y + line_height < clip_rect.y && !word_wrap_enabled)
		while (y + line_height < clip_rect.y && s < text_end) {
			s = (const char*)memchr(s, '\n', text_end - s);
			s = s ? s + 1 : text_end;
			y += line_height;
		}

	// Align
	float adjust = 0;

	if (align != -1) {
		for (const char *s = text_begin; s < text_end;) {
			unsigned int c = (unsigned int)*s;
			if (c < 0x80) {
				++s;
			} else {
				s += ImTextCharFromUtf8(&c, s, text_end);
				if (!c)
					break;
			}

			if (c == '\r')
				continue;

			if (c == '\n')
				break;

			const ImFontGlyph *glyph = font->FindGlyph((ImWchar)c);
			if (!glyph)
				continue;

			adjust += glyph->AdvanceX * scale;
		}

		if (align == 0)
			adjust *= 0.5f;
	}

	for (const char *s = text_begin; s < text_end;) {
		if (word_wrap_enabled) {
			if (!word_wrap_eol) {
				word_wrap_eol = font->CalcWordWrapPositionA(scale, s, text_end, wrap_width - (x - pos.x));

				if (word_wrap_eol == s)
					++word_wrap_eol;
			}

			if (s >= word_wrap_eol) {
				x = pos.x;
				y += line_height;
				word_wrap_eol = NULL;

				// Wrapping skips upcoming blanks
				while (s < text_end) {
					const char *c = s;
					if (*c == ' ' || *c == '\t') {
						++s;
					} else {
						if (*c == '\n')
							++s;
						break;
					}
				}
				continue;
			}
		}

		// Decode and advance source
		unsigned int c = (unsigned int)*s;
		if (c < 0x80) {
			++s;
		} else {
			s += ImTextCharFromUtf8(&c, s, text_end);
			if (!c)
				break;
		}

		if (c < 32) {
			if (c == '\n') {
				x = pos.x;
				y += line_height;
				if (y > clip_rect.y + clip_rect.h)
					break; // break out of main loop
				continue;
			}
			if (c == '\r')
				continue;
		}

		const ImFontGlyph *glyph = font->FindGlyph((ImWchar)c);
		if (!glyph)
			continue;

		float char_width = glyph->AdvanceX * scale;
		x += char_width;

		if (x > max_width)
			max_width = x;
	}

	pos.x = x;
	pos.y = y;
	max_width -= x_orig;
}

void Hud::slow_text(int fnt_id, float size, ImVec2 pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align)
{
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	ImFont *font = fonts[fnt_id];
	glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)font->ContainerAtlas->TexID);
	text(font, size, pos, SDL_Color{0xff, 0xff, 0xff, 0xff}, clip_rect, text_begin, n, wrap_width, align);
	glDisable(GL_TEXTURE_2D);
}

void Hud::slow_text(int fnt_id, float size, ImVec2 pos, SDL_Color col, const SDL_Rect &clip_rect, const std::string &text, float wrap_width, int align)
{
	slow_text(fnt_id, size, pos, col, clip_rect, text.c_str(), text.size(), wrap_width, align);
}

void Hud::title(ImVec2 pos, SDL_Color col, const SDL_Rect &clip_rect, const char *text_begin, size_t n, float wrap_width, int align)
{
	slow_text(5, font_sizes[5], pos, col, clip_rect, text_begin, n, wrap_width, align);
}

void Button::display(Hud &hud) {
	glEnable(GL_BLEND);
	hud.shaderect(bnds, shade);
	glDisable(GL_BLEND);

	if (hot && active) {
		SDL_Color col[6];

		for (unsigned i = 0; i < 6; ++i)
			col[5 - i] = cols[i];

		hud.rect(bnds, col);
	} else {
		hud.rect(bnds, cols);
	}

	hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2((float)(left + bnds.w / 2), (float)(top + (bnds.h - fonts[fnt_id]->FontSize) / 2)), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{(int)left, (int)top, (int)(right - left), (int)(bottom - top)}, label, 200, 0);
}

bool Button::update(Hud &hud) {
	// copy state
	left = hud.left; top = hud.top; right = hud.right; bottom = hud.bottom;
	label = hud.m_text;
	shade = 2 * hud.shade;
	fnt_id = hud.fnt_id;

	for (unsigned i = 0; i < 6; ++i)
		cols[i] = hud.cols[i];

	bnds.x = static_cast<int>(left);
	bnds.y = static_cast<int>(top);
	bnds.w = static_cast<int>(right - left);
	bnds.h = static_cast<int>(bottom - top);

	hot = contains(left, top, right, bottom, hud.mouse_x, hud.mouse_y);

	if (hud.try_grab(SDL_BUTTON_LMASK, hot)) {
		active = true;
		genie::jukebox.sfx(genie::DrsId::button4);
	}

	if (hud.lost_grab(SDL_BUTTON_LMASK, active)) {
		active = false;
		selected = hot;
		return hot && enabled;
	}

	return false;
}

bool ListPopup::update(Hud &hud) {
	// copy state
	left = hud.left; top = hud.top; right = hud.right; bottom = hud.bottom;
	align_left = hud.align_left;
	shade = 2 * hud.shade;

	for (unsigned i = 0; i < 6; ++i)
		cols[i] = hud.cols[i];

	bnds.x = static_cast<int>(left);
	bnds.y = static_cast<int>(top);
	bnds.w = static_cast<int>(right - left);
	bnds.h = static_cast<int>(bottom - top);

	if (!align_left)
		left = right - 28;

	right = left + 28;
	bottom = top + 38;

	hot = contains(left, top, right, bottom, hud.mouse_x, hud.mouse_y);

	if (hud.try_grab(SDL_BUTTON_LMASK, hot)) {
		active = true;
		genie::jukebox.sfx(genie::DrsId::button4);
	}

	if (hud.lost_grab(SDL_BUTTON_LMASK, active)) {
		active = false;
		selected = hot;
		return hot && enabled;
	}

	return false;
}

Terrain::Terrain() : tiles(), overlay(), w(20), h(20)
	, left(0), right(0), bottom(1), top(1), tile_focus(-1), tile_focus_obj(nullptr)
	, tiledata(), cam(), cam_moved(false)
	, static_objects(std::bind(getStaticObjectAABB, std::placeholders::_1), std::bind(cmpStaticObject, std::placeholders::_1, std::placeholders::_2))
	, vis_static_objects(), cull_margin(-20), tex(nullptr)
	, memedit(), filename(), filedata(), scn(nullptr) { resize(w, h); }

void Terrain::build_tiles(genie::Texture &tex) {
	tiledata.clear();

	for (unsigned i = 0; i < IM_ARRAYSIZE(anim_count); ++i) {
		auto &strip = *tex.ts.animations.find(i);

		for (unsigned j = 0; j < strip.tiles.size(); ++j)
			tiledata.emplace_back((AnimationId)i, strip.tiles[j]);
	}
}

void Terrain::init(genie::Texture &tex) {
	build_tiles(tex);

	// FIXME compute proper bounds
	double hsize = (std::max<double>(right, bottom) + 640.0) / 2.0;
	genie::Box<double> bounds(hsize, 0, hsize, hsize);
	static_objects.reset(bounds, 2 + std::max<int>(0, (int)floor(log(hsize) / log(4))));

	for (int left = 0, top = 0, y = 0; y < h; ++y, left += thw, top -= thh) {
		for (int right = left, bottom = top, x = 0; x < w; ++x, right += thw, bottom += thh) {
			long long pos = (long long)y * w + x;
			uint8_t id = tiles[pos];
			int8_t height = hmap[pos];

			if (!id || id - 1 >= tiledata.size()) {
				genie::Box<double> bounds((double)right + thw, (double)bottom + thh, thw, thh);
				static_objects.try_emplace(bounds, (size_t)pos);
				continue;
			}

			double x0, y0, x1, y1;

			auto &t = tiledata[id - 1];
			auto &strip = *tex.ts.animations.find(t.id);
			auto &img = *tex.ts.images.find(t.subimage);

			x0 = right - img.bnds.x;
			x1 = x0 + img.bnds.w;
			y0 = bottom - img.bnds.y - height * thh;
			y1 = y0 + img.bnds.h;

			genie::Box<double> bounds(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0.5, (x1 - x0) * 0.5, (y1 - y0) * 0.5);
			static_objects.try_emplace(bounds, (size_t)pos);
		}
	}

	this->tex = &tex;
	tile_focus_obj = nullptr;
	tile_focus = -1;
	set_view();
}

void Terrain::invalidate_cam_focus(std::pair<StaticObject*, bool> ret, unsigned image_index, uint8_t id, int8_t height, bool update_focus) {
	if (ret.second) {
		ret.first->image_index = image_index;
		ret.first->id = id;
		ret.first->height = height;
		if (update_focus)
			tile_focus_obj = ret.first;
	} else if (update_focus) {
		tile_focus_obj = nullptr;
		tile_focus = -1;
	}
	set_view();
}

void Terrain::update_tile(const StaticObject &obj) {
	size_t pos = obj.tpos;
	uint8_t id = tiles[pos];
	int8_t height = hmap[pos];
	bool update_focus = tile_focus_obj == &obj;

	//double right = obj.bounds.center.x - obj.bounds.hsize.x;
	//double bottom = obj.bounds.center.y - obj.bounds.hsize.y;
	int y = obj.tpos / w, x = obj.tpos % w;
	// height is not added as we can compute this manually
	double right = (y + x) * thw, bottom = (x - y) * thh;
	unsigned image_index = obj.image_index;

	if (!static_objects.erase(obj))
		assert("object should be removed" == 0);

	if (!id || id - 1 >= tiledata.size()) {
		genie::Box<double> bounds((double)right + thw, (double)bottom + thh, thw, thh);
		auto ret = static_objects.try_emplace(bounds, (size_t)pos);
		invalidate_cam_focus(ret, image_index, id, height, update_focus);
		return;
	}

	double x0, y0, x1, y1;

	auto &t = tiledata[id - 1];
	auto &strip = *tex->ts.animations.find(t.id);
	auto &img = *tex->ts.images.find(t.subimage);

	x0 = right - img.bnds.x;
	x1 = x0 + img.bnds.w;
	y0 = bottom - img.bnds.y - (int)height * thh;
	y1 = y0 + img.bnds.h;

	genie::Box<double> bounds(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0.5, (x1 - x0) * 0.5, (y1 - y0) * 0.5);
	auto ret = static_objects.try_emplace(bounds, (size_t)pos);
	invalidate_cam_focus(ret, image_index, id, height, update_focus);
}

void Terrain::resize(int w, int h) {
	if (w < 1) w = 1;
	if (h < 1) h = 1;

	long long sz = (long long)w * h;
	int pad_x = 10;

	left = 0;
	right = std::max<int>(w * tw, h * th);
	bottom = w * thh;
	top = -bottom;

	tiles.resize(sz, 1);
	subimg.resize(sz, 0);
	hmap.resize(sz);
	overlay.resize(sz);

	this->w = w;
	this->h = h;
}

void Terrain::select(int mx, int my, int vpx, int vpy) {
	// compute world coordinate
	float pos[3], scr[3];

	scr[0] = (float)(mx + vpx); scr[1] = float(vp[3] - my + vpy); scr[2] = 0;
	tile_focus = -1;

	try {
		genie::unproject(pos, scr, mv, proj, vp);
	} catch (std::runtime_error &e) {
		fputs("unproject failed\n", stderr);
		return;
	}

	// determine selected tile
#if 1
	for (auto it : vis_static_objects) {
		auto &o = *it;

		if (o.tpos != (size_t)-1) {
			size_t i = o.tpos;
			uint8_t id = tiles[i];

			if (!id || id - 1 > tiledata.size())
				id = 1; // empty or invalid tiles should still be selectable

			auto &t = tiledata[id - 1];
			auto &strip = *tex->ts.animations.find(t.id);
			auto &img = *tex->ts.images.find(t.subimage);

			double left = o.bounds.center.x - o.bounds.hsize.x, top = o.bounds.center.y - o.bounds.hsize.y;

			if (img.contains((int)(pos[0] - left), (int)(pos[1] - top))) {
				tile_focus = (long long)i;
				tile_focus_obj = const_cast<StaticObject*>(it);
				break;
			}
		}
	}
#else
	for (size_t i = 0; i < tiles.size(); ++i) {
		uint8_t id = tiles[i];

		if (!id || id - 1 > tiledata.size())
			id = 1; // empty or invalid tiles should still be selectable

		auto &t = tiledata[id - 1];
		auto &strip = *tex.ts.animations.find(t.id);
		auto &img = *tex.ts.images.find(t.subimage);
		auto &gfx = tilegfx[i];

		if (img.contains((int)(pos[0] - gfx.x), (int)(pos[1] - gfx.y))) {
			tile_focus = (long long)i;
			break;
		}
	}
#endif
}

void Terrain::set_view() {
	float pos[3], scr[3];
	genie::Point<double> lt, rb;

	if (vp[2] < 1 || vp[3] < 1)
		return;

	scr[0] = (float)(vp[0] - cull_margin); scr[1] = (float)(vp[3] - vp[1] + cull_margin); scr[2] = 0;
	genie::unproject(pos, scr, mv, proj, vp);
	lt.x = pos[0]; lt.y = pos[1];

	scr[0] = (float)(vp[0] + vp[2] + cull_margin); scr[1] = (float)(vp[1] - cull_margin); scr[2] = 0;
	genie::unproject(pos, scr, mv, proj, vp);
	rb.x = pos[0]; rb.y = pos[1];

	cam.hsize.x = (rb.x - lt.x) * 0.5;
	cam.hsize.y = (rb.y - lt.y) * 0.5;
	cam.center.x = lt.x + cam.hsize.x;
	cam.center.y = lt.y + cam.hsize.y;

	auto &obj = vis_static_objects;
	obj.clear();
	static_objects.collect(obj, cam);
	cam_moved = false;
}

const std::vector<std::string> civs({
	"Assyrian",
	"Babylonian",
	"Choson",
	"Egyptian",
	"Greek",
	"Hittite",
	"Minoan",
	"Persian",
	"Phoenician",
	"Shang",
	"Sumerian",
	"Yamato",
});

const std::vector<std::string> player_count_str({"1", "2", "3", "4", "5", "6", "7", "8"});

AoE::AoE(Worker &w_bkg)
	: video(), tex_bkg(), ts_next(), dlgcol(), hud(), mouse_x(-1), mouse_y(-1)
	, working(false), global_settings(false), wip(false), w_bkg(w_bkg), terrain()
	, player_count(0)
	, lgy_index(2), left(0), right(0), top(0), bottom(0)
	, cam_x(0), cam_y(0), cam_zoom(1.0f), show_drs(false), open_dlg(false), dlg_id(NULL)
	, teams(), players(), vplayers()
{
	settings.load(video);

	if (settings.load()) {
		video.restore(settings);
	}
}

void AoE::load_menu(MenuId id) {
	ZoneScoped;
	hud.reset(tex_bkg);
	tex_bkg.ts = std::move(ts_next);
	tex_bkg.ts.write(tex_bkg.tex);

	switch (id) {
		case MenuId::scn_edit:
			terrain.init(tex_bkg);
			break;
		case MenuId::singleplayer: {
			submenu_id = SubMenuId::singleplayer_menu;
			teams.clear();
			players.clear();
			vplayers.clear();

			unsigned id, p_id;

			players.emplace_back(p_id = players.size(), 0, "default");
			vplayers.emplace_back(id = vplayers.size(), p_id, players[p_id].name);

			players.emplace_back(p_id = players.size(), 0, "You");
			vplayers.emplace_back(id = vplayers.size(), p_id, players[p_id].name);

			player_count = 0;
			break;
		}
	}

	menu_id = id;
}

void AoE::idle(SDL_Event &event) {
	ZoneScoped;

	switch (event.type) {
		case SDL_MOUSEMOTION:
			mouse_x = event.motion.x;
			mouse_y = event.motion.y;
			video.motion(hud.mouse_x, hud.mouse_y, mouse_x, mouse_y);
			break;
		case SDL_MOUSEBUTTONDOWN:
			hud.down(event.button);
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			video.motion(hud.mouse_x, hud.mouse_y, mouse_x, mouse_y);

			if (menu_is_game())
				terrain.select(hud.mouse_x, hud.mouse_y, video.vp.x, video.vp.y);
			break;
		case SDL_MOUSEBUTTONUP:
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			video.motion(hud.mouse_x, hud.mouse_y, mouse_x, mouse_y);
			hud.up(event.button);
			break;
	}
}

void AoE::draw_background() {
	ZoneScoped;
	assert(tex_bkg.tex);
	const genie::SubImage &img = tex_bkg.ts[lgy_index];

	glBegin(GL_QUADS);
	{
		glTexCoord2f(img.s0, img.t0); glVertex2f(left, top);
		glTexCoord2f(img.s1, img.t0); glVertex2f(right, top);
		glTexCoord2f(img.s1, img.t1); glVertex2f(right, bottom);
		glTexCoord2f(img.s0, img.t1); glVertex2f(left, bottom);
	}
	glEnd();
}

void AoE::draw_terrain() {
	ZoneScoped;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// center camera to make zooming and scrolling easier
	GLdouble w = right - left, h = top - bottom;

	glOrtho(-w * .5, w * .5, -h * .5, h * .5, -1, 1);
	//glOrtho(left, right, bottom, top, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(cam_zoom, cam_zoom, 1.0f);
	glTranslatef(-cam_x, -cam_y, 0);
	terrain.show(tex_bkg);

	// restore
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left - 0.5, right - 0.5, bottom - 0.5, top - 0.5, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void AoE::display_editor(SDL_Rect bnds) {
	ZoneScoped;
	hud.rect(bnds, dlgcol.bevel);

	double btn_x, btn_y, btn_w, btn_h, pad_y;

	btn_w = bnds.w * 300.0 / 640.0;
	btn_h = bnds.h * 40.0 / 480.0;
	btn_x = bnds.w * 0.5 - btn_w * 0.5;
	btn_y = bnds.h * 0.5;
	pad_y = bnds.h * 10.0 / 480.0;

	int selected = -1;

	TextId ids[] = {
		TextId::btn_scn_edit,
		TextId::btn_cpn_edit,
		TextId::btn_drs_edit,
		TextId::btn_back,
	};

	for (int i = 0; i < IM_ARRAYSIZE(ids); ++i)
		if (hud.button(i, 4, btn_x, btn_y + (i - 2) * (btn_h + pad_y), btn_w, btn_h, TXT(ids[i]), dlgcol.shade, dlgcol.bevel))
			selected = i;

	if (working)
		return;

	switch (selected) {
		case 0:
			show_drs = false;
			w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::scn_edit);
			break;
		case 2:
			show_drs = true;
			break;
		case 3:
			show_drs = false;
			w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::start);
			break;
	}
}

void AoE::display_scn_edit(SDL_Rect bnds) {
	ZoneScoped;
	SDL_Rect top, bottom;

	top.x = bnds.x; top.y = bnds.y; top.w = bnds.w; top.h = 51;
	bottom.x = bnds.x; bottom.y = bnds.h - 143; bottom.w = bnds.w; bottom.h = 143;

	double btn_x, btn_y, btn_w, btn_h, pad_x, pad_y;

	pad_x = 3; pad_y = 5;
	btn_w = 58; btn_h = 40;
	btn_x = bnds.w - btn_w - pad_x; btn_y = pad_y;

	int selected = -1;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_bkg.tex);

	glEnable(GL_BLEND);
	draw_terrain();//terrain.show(tex_bkg);

				   // NOTE y-coordinate for scissor is flipped
				   // the coordinates are window global, so we need the video viewport as well
	glEnable(GL_SCISSOR_TEST);
	glScissor(video.vp.x, video.vp.y + video.vp.h - top.h, top.w, top.h);
	draw_background();
	glScissor(video.vp.x, video.vp.y, bottom.w, bottom.h);
	draw_background();
	glDisable(GL_SCISSOR_TEST);

	glDisable(GL_TEXTURE_2D);


	hud.rect(top, dlgcol.bevel);
	hud.rect(bottom, dlgcol.bevel);

	if (hud.button(0, 0, btn_x, btn_y, btn_w, btn_h, TXT(TextId::btn_back), dlgcol.shade, dlgcol.bevel))
		selected = 0;

	btn_x = pad_x;

	if (hud.button(1, 0, btn_x, btn_y, btn_w, btn_h, TXT(TextId::btn_scn_load), dlgcol.shade, dlgcol.bevel))
		selected = 1;

	if (working)
		return;

	switch (selected) {
		case 0:
			w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::editor);
			break;
		case 1:
			open_dlg = true;
			dlg_id = FDLG_CHOOSE_SCN;
			//fd->OpenDialog(FDLG_CHOOSE_SCN, CTXT(TextId::btn_scn_load), ".scn", genie::game_dir);
			break;
	}
}

void AoE::display_start(SDL_Rect bnds) {
	ZoneScoped;
	hud.rect(bnds, dlgcol.bevel);

	double btn_x, btn_y, btn_w, btn_h, pad_y;

	btn_w = bnds.w * 300.0 / 640.0;
	btn_h = bnds.h * 40.0 / 480.0;
	btn_x = bnds.w * 0.5 - btn_w * 0.5;
	btn_y = bnds.h * 0.625;
	pad_y = bnds.h * 10.0 / 480.0;

	int selected = -1;

	TextId ids[] = {
		TextId::btn_singleplayer,
		TextId::btn_multiplayer,
		TextId::btn_main_settings,
		TextId::btn_editor,
		TextId::btn_quit,
	};

	for (int i = 0; i < 5; ++i)
		if (hud.button(i, 4, btn_x, btn_y - btn_h * 0.5 + (i - 2) * (btn_h + pad_y), btn_w, btn_h, TXT(ids[i]), dlgcol.shade, dlgcol.bevel))
			selected = i;

	if (working)
		return;

	switch (selected) {
		case 0: // single player
			w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::singleplayer);
			break;
		case 1: // multiplayer
			break;
		case 2: // help
			global_settings = true;
			break;
		case 3: // scenario builder
			w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::editor);
			break;
		case 4: // exit
			running = false;
			break;
	}
}

void AoE::display_singleplayer(SDL_Rect bnds) {
	switch (submenu_id) {
		case SubMenuId::singleplayer_game:
			display_singleplayer_game(bnds);
			break;
		default:
			display_singleplayer_menu(bnds);
			break;
	}
}

void AoE::display_singleplayer_menu(SDL_Rect bnds) {
	ZoneScoped;
	hud.rect(bnds, dlgcol.bevel);

	double btn_x, btn_y, btn_w, btn_h, pad_y;

	btn_w = bnds.w * 300.0 / 640.0;
	btn_h = bnds.h * 40.0 / 480;
	btn_x = bnds.w * 0.5 - btn_w * 0.5;
	btn_y = bnds.h * 0.6;
	pad_y = bnds.h * 10.0 / 480.0;

	int selected = -1;

	TextId ids[] = {
		TextId::btn_random_map,
		TextId::btn_missions,
		TextId::btn_deathmatch,
		TextId::btn_scenario,
		TextId::btn_restore_game,
		TextId::btn_cancel,
	};

	for (int i = 0; i < 6; ++i)
		if (hud.button(i, 4, btn_x, btn_y - btn_h * 0.5 + (i - 3) * (btn_h + pad_y), btn_w, btn_h, TXT(ids[i]), dlgcol.shade, dlgcol.bevel))
			selected = i;

	int hdr_x = bnds.w / 2, hdr_y = bnds.h * 48 / 768;

	const char *str = CTXT(TextId::title_singleplayer);
	hud.title(ImVec2(hdr_x, hdr_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{0, 0, bnds.w, 64}, str, strlen(str), bnds.w, 0);

	if (working)
		return;

	switch (selected) {
		case 0: // random map
			hud.reset();
			submenu_id = SubMenuId::singleplayer_game;
			break;
		case 5: // cancel
			w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::start);
			break;
	}
}

void AoE::display_singleplayer_game(SDL_Rect bnds) {
	ZoneScoped;
	hud.rect(bnds, dlgcol.bevel);

	double btn_x, btn_y, btn_w, btn_h, pad_x;

	pad_x = bnds.w * 20.0 / 640.0;
	btn_w = bnds.w * 240.0 / 640.0;
	btn_h = bnds.h * 30.0 / 480.0;
	btn_x = (bnds.w - pad_x) * 0.5 - btn_w;
	btn_y = bnds.h * 704.0 / 768.0;

	int selected = -1;

	if (hud.button(0, 4, btn_x, btn_y, btn_w, btn_h, TXT(TextId::btn_start_game), dlgcol.shade, dlgcol.bevel))
		selected = 0;

	btn_x = (bnds.w + pad_x) * 0.5;

	if (hud.button(1, 4, btn_x, btn_y, btn_w, btn_h, TXT(TextId::btn_cancel), dlgcol.shade, dlgcol.bevel))
		selected = 1;

	btn_x = bnds.w * 672.0 / 1024.0;
	btn_y = bnds.h * 80.0 / 768.0;
	btn_w = bnds.w * 336.0 / 1024.0;

	if (hud.button(2, 4, btn_x, btn_y, btn_w, btn_h, TXT(TextId::btn_game_settings), dlgcol.shade, dlgcol.bevel))
		selected = 2;

	int hdr_x = bnds.w / 2, hdr_y = bnds.h * 14 / 768;

	const char *str = CTXT(TextId::title_singleplayer_game);
	hud.title(ImVec2(hdr_x, hdr_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{0, 0, bnds.w, 64}, str, strlen(str), bnds.w, 0);

	btn_x = bnds.w * 49.0 / 1024.0;
	btn_y = bnds.h * 515.0 / 768.0;
	btn_w = 112.0;
	btn_h = 38.0;

	int fnt_id = 4;

	if (hud.listpopup(4, fnt_id, btn_x, btn_y, btn_w, btn_h, player_count, player_count_str, dlgcol.shade, dlgcol.bevel, false))
		selected = 4;

	int txt_x, txt_y, txt_w, txt_h;
	txt_x = bnds.w * 46 / 1024;
	txt_w = bnds.w * 270 / 1024;
	txt_h = 38;

	for (unsigned i = 1; i < vplayers.size(); ++i) {
		btn_x = bnds.w * 272.0 / 1024.0;
		btn_y = bnds.h * 134.0 / 768.0 + 38 * (i - 1);
		txt_y = bnds.h * 142 / 768 + 38 * (i - 1);

		int id;

		Player &p = players[vplayers[i].p_id];
		btn_w = bnds.w * 220.0 / 1024.0;

		hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2(txt_x, txt_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{txt_x, txt_y, txt_w, txt_h}, p.name, txt_w, -1);

		if (hud.listpopup(id = 5 + 3 * (i - 1), fnt_id, btn_x, btn_y, btn_w, btn_h, p.civ_id, civs, 255, dlgcol.bevel))
			selected = id;

		btn_x = bnds.w * 496.0 / 1024.0;
		btn_w = bnds.w * 48.0 / 1024.0;
		btn_h = 32;//bnds.h * 32.0 / 768.0;

		if (hud.button(id = 6 + 3 * (i - 1), 4, btn_x, btn_y, btn_w, btn_h, "1", dlgcol.shade, dlgcol.bevel))
			selected = id;

		btn_x = bnds.w * 608.0 / 1024.0;

		if (hud.button(id = 7 + 3 * (i - 1), 4, btn_x, btn_y, btn_w, btn_h, "-", dlgcol.shade, dlgcol.bevel))
			selected = id;
	}

	txt_y = bnds.h * 480 / 768;
	txt_w = 250;
	txt_h = 20;

	hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2(txt_x, txt_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{txt_x, txt_y, txt_w, txt_h}, TXT(TextId::player_count), txt_w, -1);

	txt_y = bnds.h * 96 / 768;
	hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2(txt_x, txt_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{txt_x, txt_y, txt_w, txt_h}, TXT(TextId::player_name), txt_w, -1);

	txt_x = bnds.w * 306 / 1024;
	txt_w = bnds.w * 168 / 1024;
	hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2(txt_x, txt_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{txt_x, txt_y, txt_w, txt_h}, TXT(TextId::player_civ), txt_w, -1);

	txt_x = bnds.w * 476 / 1024;
	txt_w = bnds.w * 130 / 1024;
	hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2(txt_x, txt_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{txt_x, txt_y, txt_w, txt_h}, TXT(TextId::player_id), txt_w, -1);

	txt_x = bnds.w * 606 / 1024;
	txt_w = bnds.w * 66 / 1024;
	hud.slow_text(fnt_id, font_sizes[fnt_id], ImVec2(txt_x, txt_y), SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{txt_x, txt_y, txt_w, txt_h}, TXT(TextId::player_team), txt_w, -1);

	if (working || selected < 0)
		return;

	switch (selected) {
		case 0: // start game
			break;
		case 1: // cancel
			hud.reset();
			submenu_id = SubMenuId::singleplayer_menu;
			//w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::singleplayer_menu);
			break;
		case 2: // settings
			break;
		case 3: // ?
			break;
		case 4: // number of players
			if (players.size() != player_count + 2) {
				size_t adjust = player_count + 2;
				if (adjust < players.size()) {
					players.erase(players.begin() + adjust, players.end());
					if (adjust < vplayers.size())
						vplayers.erase(vplayers.begin() + adjust, vplayers.end());
				} else {
					for (unsigned i = players.size(); i < adjust; ++i) {
						unsigned id, p_id;
						std::string pname(std::string("Player ") + std::to_string(i));

						players.emplace_back(p_id = players.size(), 0, pname);
						vplayers.emplace_back(id = vplayers.size(), p_id, players[p_id].name);
					}
				}
			}
			break;
		default:
			switch ((selected - 5) % 3) {
				case 0: // civ
				{
					int index = (selected - 5) / 3;
					Player &p = players[vplayers[index].p_id];
					//p.civ_id =
					break;
				}
				case 1: // controlling player
					break;
				case 2: // team
					break;
			}
			break;
	}
}

void AoE::display() {
	ZoneScoped;
	SDL_Rect bnds;

	SDL_GetWindowSize(window, &bnds.w, &bnds.h);
	SDL_GetWindowPosition(window, &bnds.x, &bnds.y);

	video.vp = video.size = bnds;

	lgy_index = 2;

	// prevent division by zero
	if (bnds.h < 1) bnds.h = 1;

	long long need_w = bnds.w, need_h = bnds.h;
	right = bottom = 1024;

	if (video.aspect) {
		need_w = bnds.w;
		need_h = need_w * 3 / 4;

		// if too big, project width onto aspect ratio of height
		if (need_h > bnds.h) {
			need_h = bnds.h;
			need_w = need_h * 4 / 3;
		}

		right = need_w;
		bottom = need_h;
	} else {
		right = bnds.w;
		bottom = bnds.h;
	}

	// find closest background to match dimensions
	if (video.legacy) {
		lgy_index = 2;

		if (need_w <= 800)
			lgy_index = bnds.w <= 640 ? 0 : 1;
	}

	// adjust viewport if aspect ratio differs
	if (need_w != bnds.w || need_h != bnds.h) {
		bnds.x = (bnds.w - need_w) / 2;
		bnds.y = (bnds.h - need_h) / 2;

		glViewport(video.vp.x = bnds.x, video.vp.y = bnds.y, video.vp.w = need_w, video.vp.h = need_h);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		video.vp.x = video.vp.y = 0;
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// glVertex2i will place coordinate on top-left of pixel, adjust orthogonal view to place them on the pixel's center
	glOrtho(left - 0.5, right - 0.5, bottom - 0.5, top - 0.5, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glDisable(GL_BLEND);

	if (menu_id != MenuId::config) {
		bnds.x = left; bnds.y = top; bnds.w = right - left; bnds.h = bottom - top;

		if (menu_id != MenuId::scn_edit) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, tex_bkg.tex);
			draw_background();
			glDisable(GL_TEXTURE_2D);
		}

		hud.was_hot = false;

		switch (menu_id) {
			case MenuId::start:
				display_start(bnds);
				break;
			case MenuId::editor:
				display_editor(bnds);
				break;
			case MenuId::scn_edit:
				display_scn_edit(bnds);
				break;
			case MenuId::singleplayer:
				display_singleplayer(bnds);
				break;
		}

		hud.display();
	}
}

void ListPopup::display(Hud &hud) {
	glEnable(GL_BLEND);
	//hud.shaderect(bnds, shade);

	SDL_Rect btn_bnds{bnds.x, bnds.y, btn_w, btn_h};

	if (!align_left)
		btn_bnds.x = bnds.x + bnds.w - 28;


	SDL_Color col[6];

	for (unsigned i = 0; i < 6; ++i)
		col[5 - i] = cols[i];

	SDL_Rect shade_bnds{bnds.x, bnds.y, bnds.w - btn_w - 2, bnds.h};

	if (align_left)
		shade_bnds.x += btn_w + 2;

	if (shade < 255)
		hud.shaderect(shade_bnds, shade);

	glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, eng->tex_bkg.tex);

	auto search = eng->anims.find(AnimationId::ui_buttons);
	assert(search != eng->anims.end());

	auto &strip = *hud.tex->ts.animations.find(search->second);

	auto &img = *hud.tex->ts.images.find(strip.tiles[10]);

	GLfloat x0, y0, x1, y1;
	GLfloat s0, t0, s1, t1;

	x0 = btn_bnds.x; x1 = x0 + btn_w;
	y0 = btn_bnds.y; y1 = y0 + btn_h;

	int offx = (img.bnds.w - btn_w) / 2, offy = (img.bnds.h - btn_h) / 2;

	s0 = (img.tx + offx) / (GLfloat)hud.tex->ts.bnds.w;
	s1 = (img.tx + offx + btn_w) / (GLfloat)hud.tex->ts.bnds.w;

	t0 = (img.ty + offy) / (GLfloat)hud.tex->ts.bnds.h;
	t1 = (img.ty + offy + btn_h) / (GLfloat)hud.tex->ts.bnds.h;

	glBegin(GL_QUADS);
	glColor3f(1, 1, 1);
	glTexCoord2f(s0, t0); glVertex2f(x0, y0);
	glTexCoord2f(s1, t0); glVertex2f(x1, y0);
	glTexCoord2f(s1, t1); glVertex2f(x1, y1);
	glTexCoord2f(s0, t1); glVertex2f(x0, y1);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	if (hot && active) {
		hud.rect(btn_bnds, col);
	} else {
		hud.rect(btn_bnds, cols);
	}

	if (!opened)
		hud.slow_text(
			fnt_id, font_sizes[fnt_id],
			ImVec2((float)shade_bnds.x + 6, (float)shade_bnds.y + (btn_h - fonts[fnt_id]->FontSize) * 0.5f),
			SDL_Color{0xff, 0xff, 0xff, 0xff}, bnds, items[selected_item], shade_bnds.w, -1
		);
}

void ListPopup::display_popup(Hud &hud) {
	SDL_Rect shade_bnds{bnds.x, bnds.y, bnds.w - btn_w - 2, bnds.h};

	if (align_left)
		shade_bnds.x += btn_w + 2;

	ImFont *font = fonts[fnt_id];
	float pos_x = (float)shade_bnds.x + 6.0f;
	ImVec2 pos(pos_x, (float)shade_bnds.y + (btn_h - font_sizes[fnt_id]) * 0.5f);

	GLfloat left = pos_x - 10.0f, top = pos.y - 8.0f, right = left, bottom = top;

	for (unsigned i = 0; i < items.size(); ++i) {
		float w = 0;
		hud.textdim(font, font_sizes[fnt_id], pos, SDL_Rect{0, 0, eng->video.vp.w, eng->video.vp.h}, items[i], eng->video.vp.w, -1, w);
		if (pos.x > right)
			right = pos.x;
		pos.x = pos_x;
		pos.y += font_sizes[fnt_id];
	}

	right += 10.0f;
	bottom = pos.y;

	hud.shaderect(SDL_Rect{(int)left, (int)top, (int)(right - left), (int)(bottom - top)}, 120);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)font->ContainerAtlas->TexID);

	pos_x = (float)shade_bnds.x + 6.0f;
	pos = ImVec2(pos_x, (float)shade_bnds.y + (btn_h - font_sizes[fnt_id]) * 0.5f);

	item_bounds.resize(items.size());

	for (unsigned i = 0; i < items.size(); ++i) {
		item_bounds[i].x = (int)pos_x;
		item_bounds[i].y = (int)pos.y;

		float w = 0;

		hud.text(font, font_sizes[fnt_id], pos, SDL_Color{0xff, 0xff, 0xff, 0xff}, SDL_Rect{0, 0, eng->video.vp.w, eng->video.vp.h}, items[i], eng->video.vp.w, -1, w);
		pos.x = pos_x;
		pos.y += font_sizes[fnt_id];

		item_bounds[i].w = w;//(int)pos.x - item_bounds[i].x;
		item_bounds[i].h = (int)pos.y - item_bounds[i].y;
	}

	glDisable(GL_TEXTURE_2D);
}

void Terrain::load() {
	resize(scn->map_width, scn->map_height);

	// determine bounds
	build_tiles(*tex);

	int min_top = 0, max_right = 0, max_bottom = 0;

	for (int left = 0, top = 0, y = 0; y < scn->map_height; ++y, left += thw, top -= thh) {
		for (int right = left, bottom = top, x = 0; x < scn->map_width; ++x, right += thw, bottom += thh) {
			size_t pos = (size_t)y * scn->map_width + x;

#if 0
			uint8_t id = scn->tiles[pos];
			int8_t height = scn->hmap[pos];

			// TODO id mapping

			if (!id || id >= tiledata.size() - 1)
				continue;

			auto &t = tiledata[id - 1];
			auto &strip = *tex->ts.animations.find(t.id);
			auto &img = *tex->ts.images.find(t.subimage);
#else
			uint8_t id = scn->tiles[pos], subimg = scn->subimg[pos];
			int8_t height = scn->hmap[pos];

			auto search = eng->anims.find((AnimationId)((unsigned)AnimationId::desert + id - 1));
			if (search == eng->anims.end())
				continue;

			auto &strip = *tex->ts.animations.find(search->second);

			if (subimg >= strip.tiles.size())
				continue;

			auto &img = *tex->ts.images.find(strip.tiles[subimg]);

#endif
			double x0, y0, x1, y1;

			x0 = right - img.bnds.x;
			x1 = x0 + img.bnds.w;
			y0 = bottom - img.bnds.y - (int)height * thh;
			y1 = y0 + img.bnds.h;

			if (y0 < min_top)
				min_top = y0;

			if (y1 > max_bottom)
				max_bottom = y1;

			if (x1 > max_right)
				max_right = x1;
		}
	}

	double hsize = (std::max<double>(max_right, std::max<double>(-min_top, max_bottom))) / 2.0;
	genie::Box<double> bounds(hsize, 0, hsize, hsize);
	static_objects.reset(bounds, 2 + std::max<int>(0, (int)floor(log(hsize) / log(4))));

	tiles = scn->tiles;
	subimg = scn->subimg;
	hmap = scn->hmap;
	overlay = scn->overlay;

	for (int left = 0, top = 0, y = 0; y < h; ++y, left += thw, top -= thh) {
		for (int right = left, bottom = top, x = 0; x < w; ++x, right += thw, bottom += thh) {
			long long pos = (long long)y * w + x;
			uint8_t id = tiles[pos];
			int8_t height = hmap[pos];

			if (!id || id - 1 >= tiledata.size()) {
				printf("%d,%d: %" PRIu8 ", %" PRId8 ", %" PRIu8 "\n", x, y, tiles[pos], hmap[pos], scn->overlay[pos]);
				genie::Box<double> bounds((double)right + thw, (double)bottom + thh, thw, thh);
				static_objects.try_emplace(bounds, (size_t)pos);
				continue;
			}

			double x0, y0, x1, y1;

			auto &t = tiledata[id - 1];
			auto &strip = *tex->ts.animations.find(t.id);
			auto &img = *tex->ts.images.find(t.subimage);

			x0 = right - img.bnds.x;
			x1 = x0 + img.bnds.w;
			y0 = bottom - img.bnds.y - height * thh;
			y1 = y0 + img.bnds.h;

			genie::Box<double> bounds(x0 + (x1 - x0) * 0.5, y0 + (y1 - y0) * 0.5, (x1 - x0) * 0.5, (y1 - y0) * 0.5);
			static_objects.try_emplace(bounds, (size_t)pos);
		}
	}

	tile_focus_obj = nullptr;
	tile_focus = -1;
	set_view();
}

void Terrain::show(genie::Texture &tex) {
	ZoneScoped;
	// save unproject data
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);
	glGetFloatv(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, vp);

	if (cam_moved)
		set_view();

	glBegin(GL_QUADS);
	{
		for (auto it : vis_static_objects) {
			auto &o = *it;

			// if is tile
			if (o.tpos != (size_t)-1) {
				GLfloat x0, y0, x1, y1;

				auto pos = o.tpos;
#if 0
				uint8_t id = tiles[pos];
				int8_t height = hmap[pos];

				if (!id || id - 1 >= tiledata.size())
					continue;

				auto &t = tiledata[id - 1];
				auto &strip = *tex.ts.animations.find(t.id);
				auto &img = *tex.ts.images.find(t.subimage);
#else
				uint8_t id = tiles[pos], sub = subimg[pos];
				int8_t height = hmap[pos];

				auto search = eng->anims.find((AnimationId)((unsigned)AnimationId::desert + id - 1));
				if (search == eng->anims.end())
					continue;

				auto &strip = *tex.ts.animations.find(search->second);

				if (sub >= strip.tiles.size())
					continue;

				auto &img = *tex.ts.images.find(strip.tiles[sub]);
#endif

				x0 = o.bounds.center.x - o.bounds.hsize.x; x1 = o.bounds.center.x + o.bounds.hsize.x;
				y0 = o.bounds.center.y - o.bounds.hsize.y; y1 = o.bounds.center.y + o.bounds.hsize.y;

				glTexCoord2f(img.s0, img.t0); glVertex2f(x0, y0);
				glTexCoord2f(img.s1, img.t0); glVertex2f(x1, y0);
				glTexCoord2f(img.s1, img.t1); glVertex2f(x1, y1);
				glTexCoord2f(img.s0, img.t1); glVertex2f(x0, y1);
			}
		}
	}
	glEnd();

	if (show_debug) {
		glDisable(GL_TEXTURE_2D);
		static_objects.show();
		glColor4f(0, 1, 0, 0.3);
		glBegin(GL_QUADS);
		double left, top, right, bottom;
		left = cam.center.x - cam.hsize.x;
		top = cam.center.y - cam.hsize.y;
		right = cam.center.x + cam.hsize.x;
		bottom = cam.center.y + cam.hsize.y;
		glVertex2f(left, top);
		glVertex2f(right, top);
		glVertex2f(right, bottom);
		glVertex2f(left, bottom);
		glEnd();
		glColor3f(1, 1, 1);
		glEnable(GL_TEXTURE_2D);
	}
}

}

// Main code
int main(int, char**)
{
	genie::t_main = std::this_thread::get_id();
	// Setup SDL
	// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
	// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("AoE free software Remake v0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dim_lgy[2].w, dim_lgy[2].h, window_flags);
    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

	SDL_SetWindowMinimumSize(window, dim_lgy[0].w, dim_lgy[0].h);

	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &genie::max_texture_size);

	if (genie::max_texture_size < 64) {
		GLint def = 1024;
		fprintf(stderr, "bogus max texture size: %d\nusing default: %d\n", genie::max_texture_size, def);
		genie::max_texture_size = def;
	}

	time_t t_start = time(NULL);
	struct tm *tm_start = localtime(&t_start);
	int year_start = std::max(tm_start->tm_year, 2020);

	if (Mix_Init(0) == -1) {
		fprintf(stderr, "fatal error: mix_init: %s\n", Mix_GetError());
		return 1;
	}

	int frequency = 44100;
	Uint32 format = MIX_DEFAULT_FORMAT;
	int channels = 2;
	int chunksize = 1024;

	if (Mix_OpenAudio(frequency, format, channels, chunksize)) {
		fprintf(stderr, "fatal error: mix_openaudio: %s\n", Mix_GetError());
		return 1;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL2_Init();

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());

	// Our state
	bool show_demo_window = false;
	bool ther_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	imgui_addons::ImGuiFileBrowser fd;

	bool fs_choose_root = false, auto_detect = false;
	genie::GLchannel glch;
	genie::Worker w_bkg(glch);
	int err_bkg;

	std::thread t_bkg(worker_init, std::ref(w_bkg), std::ref(err_bkg));
	genie::WorkerProgress p = { 0 };
	std::string bkg_result;

	genie::AoE aoe(w_bkg);
	genie::eng = &aoe;
	genie::VideoControl &video = aoe.video;
	genie::ui::UI ui;

	// some stuff needs to be cleaned up before we shut down, so we need another scope here
	{
		std::stack<const char*> auto_paths;
#if windows
		auto_paths.emplace("C:\\Program Files (x86)\\Microsoft Games\\Age of Empires");
		auto_paths.emplace("C:\\Program Files\\Microsoft Games\\Age of Empires");
#else
		auto_paths.emplace("/media/cdrom");
#endif

		if (genie::settings.gamepath.length())
			auto_paths.emplace(genie::settings.gamepath.c_str());

		if (!auto_paths.empty()) {
			auto_detect = true;
			std::string path(auto_paths.top());
			w_bkg.tasks.produce(genie::WorkTaskType::check_root, std::make_pair(path, path));
			auto_paths.pop();
		}

		DrsView drsview;
		std::unique_ptr<UnitView> unitview;

		// Main loop
		while (running)
		{
			// Poll and handle events (inputs, window resize, etc.)
			// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
			// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
			// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
			// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				bool munched, p;

				// check if imgui munches event
				switch (event.type) {
				case SDL_MOUSEWHEEL:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP:
					p = ImGui_ImplSDL2_ProcessEvent(&event);
					munched = p && io.WantCaptureMouse;
					break;
				case SDL_TEXTINPUT:
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					p = ImGui_ImplSDL2_ProcessEvent(&event);
					munched = p && io.WantCaptureKeyboard;
					break;
				default:
					munched = ImGui_ImplSDL2_ProcessEvent(&event);
					break;
				}

				if (event.type == SDL_QUIT)
					running = false;
				if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
					running = false;

				if (!munched) {
					switch (event.type) {
					case SDL_KEYUP:
						switch (event.key.keysym.sym) {
							case SDLK_BACKQUOTE:
								show_debug = !show_debug;
								ui.show_debug(!ui.show_debug());
								break;
							case SDLK_F11:
								video.set_fullscreen(!video.fullscreen);
								break;
						}
						break;
					default:
						aoe.idle(event);
						break;
					}
				}
			}

			video.idle();
			aoe.working = w_bkg.progress(p);

			// munch all OpenGL commands
			for (std::optional<genie::GLcmd> cmd; cmd = glch.cmds.try_consume(), cmd.has_value();) {
				switch (cmd->type) {
					case genie::GLcmdType::stop:
						break;
					case genie::GLcmdType::set_bkg:
						aoe.ts_next = std::move(std::get<genie::Tilesheet>(cmd->data));
						break;
					case genie::GLcmdType::set_dlgcol:
						aoe.dlgcol = std::move(std::get<genie::DialogColors>(cmd->data));
						break;
					case genie::GLcmdType::set_anims:
						aoe.anims = std::move(std::get<std::map<genie::AnimationId, unsigned>>(cmd->data));
						break;
					default:
						assert("bad opengl command" == 0);
						break;
				}
			}

			// munch all results
			for (std::optional<genie::WorkResult> res; res = w_bkg.results.try_consume(), res.has_value();) {
				switch (res->type) {
					case genie::WorkResultType::success:
						switch (res->task.type) {
							case genie::WorkTaskType::stop:
								break;
							case genie::WorkTaskType::check_root:
								// build font cache
								io.Fonts->Clear();
								io.Fonts->AddFontDefault();

								//for (auto &p : genie::assets.ttf) {
								for (unsigned i = 0; i < 6; ++i) {
									auto &p = genie::assets.ttf[i];
									void *copy = IM_ALLOC(p->length());
									memcpy(copy, p->get(), p->length());
									fonts[i] = io.Fonts->AddFontFromMemoryTTF(copy, p->length(), font_sizes[i]);
								}

								ImGui_ImplOpenGL2_CreateFontsTexture();

								fs_has_root = true;
								// start game automatically on auto detect
								if (auto_detect) {
									auto_detect = false;
									w_bkg.tasks.produce(genie::WorkTaskType::load_menu, genie::MenuId::start);
								}

								unitview.reset(new UnitView());
								break;
							case genie::WorkTaskType::load_menu:
								aoe.load_menu(std::get<genie::MenuId>(res->task.data));
								break;
							default:
								assert("bad task type" == 0);
								break;
						}
						break;
					case genie::WorkResultType::stop:
						break;
					case genie::WorkResultType::fail:
						// try next one
						if (!auto_paths.empty()) {
							std::string path(auto_paths.top());
							w_bkg.tasks.produce(genie::WorkTaskType::check_root, std::make_pair(path, path));
							auto_paths.pop();
							aoe.working = true;
						} else {
							// bail out, let user decide
							auto_detect = false;
						}
						break;
					default:
						assert("bad result type" == 0);
						break;
				}
				bkg_result = res->what;
			}

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplSDL2_NewFrame(window);
			ImGui::NewFrame();

			// disable saving imgui.ini
			io.IniFilename = NULL;

			if (unitview.get())
				unitview->show();

			ui.display();

			if (show_debug) {
				if (fs_has_root)
					ImGui::PushFont(fonts[4]);

				// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
				if (show_demo_window)
					ImGui::ShowDemoWindow(&show_demo_window);

				if (ImGui::Begin("Debug control")) {
					ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
					ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

					if (ImGui::BeginTabBar("dbgtabs")) {
						if (fs_has_root && ImGui::BeginTabItem("DRS view")) {
							drsview.show();
							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("Audio")) {
							int opened, freq, ch, used;
							Uint16 fmt;
							opened = Mix_QuerySpec(&freq, &fmt, &ch);
							used = Mix_Playing(-1);

							if (opened <= 0) {
								ImGui::TextUnformatted("System disabled");
							} else {
								const char *str = "???";

								switch (fmt) {
									case AUDIO_U8: str = "U8"; break;
									case AUDIO_S8: str = "S8"; break;
									case AUDIO_U16LSB: str = "U16LSB"; break;
									case AUDIO_S16LSB: str = "S16LSB"; break;
									case AUDIO_U16MSB: str = "U16MSB"; break;
									case AUDIO_S16MSB: str = "S16MSB"; break;
								}

								ImGui::Text("Currently used channels: %d\n\nConfiguration:\nSystems: %d\nFrequency: %dHz\nChannels: %d\nFormat: %s", used, opened, freq, ch, str);
							}

							static bool sfx_on, msc_on;
							sfx_on = genie::jukebox.sfx_enabled();
							msc_on = genie::jukebox.msc_enabled();

							// enable/disable audio
							if (ImGui::Checkbox("Sfx", &sfx_on))
								genie::jukebox.sfx(sfx_on);
							ImGui::SameLine();
							if (ImGui::Checkbox("Music", &msc_on))
								genie::jukebox.msc(msc_on);

							// volume control
							static float sfx_vol = 100.0f, msc_vol = 100.0f;
							int old_vol = genie::jukebox.sfx_volume(), new_vol;

							sfx_vol = old_vol * 100.0f / genie::sfx_max_volume;
							ImGui::SliderFloat("Sfx volume", &sfx_vol, 0, 100.0f, "%.0f");

							sfx_vol = genie::clamp(0.0f, sfx_vol, 100.0f);
							new_vol = sfx_vol * genie::sfx_max_volume / 100.0f;

							if (old_vol != new_vol)
								genie::jukebox.sfx_volume(new_vol);

							old_vol = genie::jukebox.msc_volume();
							msc_vol = old_vol * 100.0f / genie::sfx_max_volume;
							ImGui::SliderFloat("Music volume", &msc_vol, 0, 100.0f, "%.0f");

							msc_vol = genie::clamp(0.0f, msc_vol, 100.0f);
							new_vol = msc_vol * genie::sfx_max_volume / 100.0f;

							if (old_vol != new_vol)
								genie::jukebox.msc_volume(new_vol);

							// show list of music files
							static int music_id = 0;
							genie::MusicId id;

							if (genie::jukebox.is_playing(id) == 1)
								music_id = (int)id;
							else
								music_id = 0;

							if (ImGui::Combo("Background tune", &music_id, genie::msc_names, genie::msc_count)) {
								id = (genie::MusicId)music_id;
								w_bkg.tasks.produce(genie::WorkTaskType::play_music, id);
							}

							if (ImGui::Button("Panic")) {
								genie::jukebox.stop(-1);
								genie::jukebox.stop();
							}

							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("Video")) {
							ImGui::Text("Window size: %.0fx%.0f\n", io.DisplaySize.x, io.DisplaySize.y);
							ImGui::Text("Viewport: (%d,%d), (%d,%d)", video.vp.x, video.vp.y, video.vp.x + video.vp.w, video.vp.y + video.vp.h);
							ImGui::Text("Video size: %d,%d by %dx%d", video.size.x, video.size.y, video.size.w, video.size.h);
							ImGui::Text("Mouse pos: %d,%d (p: %.1f,%.1f)", aoe.mouse_x, aoe.mouse_y, aoe.hud.mouse_x, aoe.hud.mouse_y);
							ImGui::Checkbox("Keep aspect ratio", &video.aspect);

							if (ImGui::Checkbox("Fullscreen", &video.fullscreen))
								video.set_fullscreen(video.fullscreen);

							if (ImGui::Button("Center"))
								video.center();

							if (video.displays < 1) {
								ImGui::TextUnformatted("Could not get display info");
							} else {
								ImGui::Text("Display count: %d\n", video.displays);

								str_dim_lgy[3] = CTXT(TextId::dim_custom);

								if (ImGui::Combo("Display mode", &video.mode, str_dim_lgy, IM_ARRAYSIZE(str_dim_lgy))) {
									if (video.mode < 3)
										SDL_SetWindowSize(window, dim_lgy[video.mode].w, dim_lgy[video.mode].h);
								}

								if (video.displays > 1) {
									ImGui::SliderInputInt("Display", &video.display, 0, video.displays - 1);
									video.display = genie::clamp(0, video.display, video.displays - 1);
								} else {
									video.display = 0;
								}

								const SDL_Rect &bnds = video.bounds[video.display];
								ImGui::Text("Size: %dx%d\nLocation: %d,%d\n", bnds.w, bnds.h, bnds.x, bnds.y);
							}

							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("Crash")) {
							static int current_fault = 0;
							static const char *fault_types[] = {"Segmentation violation", "NULL dereference", "_Exit", "exit", "abort", "std::terminate", "throw int"};

							ImGui::Combo("Fault type", &current_fault, fault_types, IM_ARRAYSIZE(fault_types));

							if (ImGui::Button("Oops")) {
								switch (current_fault) {
									default: raise(SIGSEGV); break;
									// NULL dereferencing directly is usually optimised out in modern compilers, so use a trick to bypass this...
									case 1: { int *p = (int*)1; -1[p]++; } break;
									case 2: _Exit(1); break;
									case 3: exit(1); break;
									case 4: abort(); break;
									case 5: std::terminate(); break;
									case 6: throw 42; break;
								}
							}
							ImGui::EndTabItem();
						}
						ImGui::EndTabBar();
					}
				}
				ImGui::End();

				if (fs_has_root)
					ImGui::PopFont();
			}

			if (aoe.show_drs) {
				ImGui::Begin("DRS view", &aoe.show_drs);
				drsview.show();
				ImGui::End();
			}

			static int lang_current = 0;
			static bool show_about = false;

			switch (menu_id) {
				case genie::MenuId::config:
					ImGui::Begin("Startup configuration", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
					{
						ImGui::SetWindowSize(io.DisplaySize);
						ImGui::SetWindowPos(ImVec2());

						if (ImGui::Button(CTXT(TextId::btn_about)))
							show_about = true;

						ImGui::TextWrapped(CTXT(TextId::hello));

						lang_current = int(lang);
						ImGui::Combo(CTXT(TextId::language), &lang_current, langs, int(LangId::max));
						lang = (LangId)genie::clamp(0, lang_current, int(LangId::max) - 1);

						static bool working;

						working = aoe.working;

						if (!aoe.working && ImGui::Button(CTXT(TextId::set_game_dir))) {
							ImGui::OpenPopup(FDLG_CHOOSE_DIR);
							puts("open");
						}

						if (fd.showFileDialog(FDLG_CHOOSE_DIR, imgui_addons::ImGuiFileBrowser::DialogMode::SELECT, ImVec2(400, 200))) {
							std::string fname(fd.selected_fn), path(fd.selected_path);
							printf("fname = %s\npath = %s\n", fname.c_str(), path.c_str());

							fs_has_root = false;
							genie::settings.gamepath = path;
							w_bkg.tasks.produce(genie::WorkTaskType::check_root, std::make_pair(fname, path));
						}

						if (aoe.working != working)
							printf("work %d -> %d\n", working, aoe.working);

						if (!bkg_result.empty())
							ImGui::TextUnformatted(bkg_result.c_str());

						if (ImGui::Button(CTXT(TextId::btn_quit)))
							running = false;

						if (fs_has_root) {
							ImGui::SameLine();

							if (ImGui::Button(CTXT(TextId::btn_startup)))
								w_bkg.tasks.produce(genie::WorkTaskType::load_menu, genie::MenuId::start);
						}
					}
					ImGui::End();
					break;
				case genie::MenuId::scn_edit:
					ImGui::Begin("Map Editor");
					{
						static int w = 20, h = 20;

						aoe.terrain.cam_moved |= ImGui::DragFloat("Cam X", &aoe.cam_x, 1.0f, aoe.terrain.left, aoe.terrain.right);
						aoe.terrain.cam_moved |= ImGui::DragFloat("Cam Y", &aoe.cam_y, 1.0f, aoe.terrain.top, aoe.terrain.bottom);
						aoe.terrain.cam_moved |= ImGui::DragFloat("Cam Zoom", &aoe.cam_zoom, 0.005f, aoe.zoom_min, aoe.zoom_max, "%.2f", 1.2f);
						aoe.cam_zoom = genie::clamp(aoe.zoom_min, aoe.cam_zoom, aoe.zoom_max);

						if (ImGui::BeginTabBar("EditorTabs")) {
							if (ImGui::BeginTabItem("Map creator")) {
								ImGui::InputInt("Map width", &w);
								ImGui::InputInt("Map height", &h);

								w = genie::clamp(1, w, 1024);
								h = genie::clamp(1, h, 1024);

								if (ImGui::Button("Create")) {
									aoe.terrain.resize(w, h);
									aoe.terrain.init(aoe.tex_bkg);
								}

								ImGui::EndTabItem();
							}

							if (ImGui::BeginTabItem("Tile Editor")) {
								ImGui::Text("Static objects: %zu/%zu", aoe.terrain.vis_static_objects.size(), aoe.terrain.static_objects.count);
								double left, top, right, bottom;
								auto cam = aoe.terrain.cam;

								left = cam.center.x - cam.hsize.x;
								top = cam.center.y - cam.hsize.y;
								right = cam.center.x + cam.hsize.x;
								bottom = cam.center.y + cam.hsize.y;

								aoe.terrain.cam_moved |= ImGui::SliderFloat("Margin", &aoe.terrain.cull_margin, -300, 300, "%.1f", 1.2);

								ImGui::Text("Frustum: (%g,%g)  (%g,%g)" , left, top, right, bottom);

								if (aoe.terrain.tile_focus == -1) {
									ImGui::TextUnformatted("Nothing selected");
								} else {
									static int id, sub, height, overlay;
									int x, y;
									long long pos = aoe.terrain.tile_focus;

									y = (int)(pos / aoe.terrain.w);
									x = (int)(pos % aoe.terrain.w);

									ImGui::Text("pos: %d,%d", x, y);

									id = aoe.terrain.tiles[pos];
									sub = aoe.terrain.subimg[pos];
									height = aoe.terrain.hmap[pos];
									overlay = aoe.terrain.overlay[pos];

									bool changed = false;

									changed |= ImGui::SliderInputInt("ID", &id, 0, UINT8_MAX);
									changed |= ImGui::SliderInputInt("Subimage", &sub, 0, UINT8_MAX);
									changed |= ImGui::InputInt("Height", &height);
									changed |= ImGui::InputInt("Overlay", &overlay);
									height = genie::clamp<int>(INT8_MIN, height, INT8_MAX);

									aoe.terrain.tiles[pos] = id;
									aoe.terrain.subimg[pos] = sub;
									aoe.terrain.hmap[pos] = height;
									aoe.terrain.overlay[pos] = overlay;

									if (changed && aoe.terrain.tile_focus_obj)
										aoe.terrain.update_tile(*aoe.terrain.tile_focus_obj);
								}

								if (ImGui::Button("Fill empty/invalid tiles")) {
									for (int y = 0; y < aoe.terrain.h; ++y) {
										for (int x = 0; x < aoe.terrain.w; ++x) {
											long long pos = (long long)y * aoe.terrain.w + x;
											uint8_t id = aoe.terrain.tiles[pos];
											int8_t height = aoe.terrain.hmap[pos];

											if (!id || id - 1 >= aoe.terrain.tiledata.size()) {
												aoe.terrain.tiles[pos] = 1;

												// choose max of direct neighbors (a tile always has two or more neighbors)
												int max = INT8_MIN;

												if (x > 0) max = std::max<int>(max, aoe.terrain.hmap[pos - 1]);
												if (x + 1 < aoe.terrain.w) max = std::max<int>(max, aoe.terrain.hmap[pos + 1]);
												if (y > 0) max = std::max<int>(max, aoe.terrain.hmap[pos - aoe.terrain.w]);
												if (y + 1 < aoe.terrain.h) max = std::max<int>(max, aoe.terrain.hmap[pos + aoe.terrain.w]);

												aoe.terrain.hmap[pos] = max;
											}
										}
									}
								}

								ImGui::EndTabItem();
							}

							ImGui::EndTabBar();
						}
					}
					ImGui::End();

					if (aoe.open_dlg) {
						aoe.open_dlg = false;
						ImGui::OpenPopup(FDLG_CHOOSE_SCN);
					}

					if (fd.showFileDialog(FDLG_CHOOSE_SCN, imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(400, 200))) {
						std::string fname(fd.selected_path);
						printf("fname = %s\n", fname.c_str());

						// open scenario
						try {
							std::ifstream in(fname, std::ios::binary);
							in.exceptions(std::ofstream::failbit | std::ofstream::badbit);

							std::streampos fsize = in.tellg();
							in.seekg(0, std::ios::end);
							fsize = in.tellg() - fsize;
							in.seekg(0, std::ios::beg);

							aoe.terrain.filedata.resize(fsize);
							in.read((char*)aoe.terrain.filedata.data(), fsize);

							aoe.terrain.filename = fname;
							aoe.terrain.scn.reset(new genie::io::Scenario(aoe.terrain.filedata));
							aoe.terrain.load();
						} catch (std::ios_base::failure &f) {
							fprintf(stderr, "scn load: %s\n", f.what());
						} catch (std::runtime_error &e) {
							fprintf(stderr, "scn load: %s\n", e.what());
						}
					}

					if (aoe.terrain.scn) {
						aoe.terrain.memedit.DrawWindow("Filedata", aoe.terrain.scn->data.data(), aoe.terrain.scn->data.size());

						ImGui::Begin("Scenario info");
						{
							ImGui::SetWindowSize(ImVec2(360, 200));
							auto *scn = aoe.terrain.scn.get();

							char v[5];
							strncpy(v, scn->hdr->version, 4);
							v[4] = 0;

							ImGui::Text("Version: %s", v);
							ImGui::Text("Header size: %" PRIX32, scn->hdr->header_size);

							ImGui::Text("Players: %" PRIu32, scn->hdr2->player_count);
							ImGui::TextWrapped("Description: %s", scn->description.c_str());

							ImGui::Text("Sections: %u", scn->sections.size());
							for (auto p : scn->sections)
								ImGui::Text("Offset: %8" PRIX64 "  Size: %8" PRIX64, (uint64_t)p.first, (uint64_t)p.second);
						}
						ImGui::End();
					}
					break;
			}

			if (show_about) {
				ImGui::Begin("About", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
				ImGui::SetWindowSize(io.DisplaySize);
				ImGui::SetWindowPos(ImVec2());
				ImGui::TextWrapped("Copyright (c) 2016-%d Folkert van Verseveld. Some rights reserved\n", year_start);
				ImGui::TextWrapped(CTXT(TextId::about));
				ImGui::TextWrapped("Using OpenGL version: %d.%d", major, minor);

				if (ImGui::Button(CTXT(TextId::btn_back)))
					show_about = false;
				ImGui::End();
			}

			static bool was_working = false, was_global_settings = false, was_wip = false;

			if (aoe.global_settings) {
				if (!was_global_settings)
					ImGui::OpenPopup(MODAL_GLOBAL_SETTTINGS);
			}

			if (ImGui::BeginPopupModal(MODAL_GLOBAL_SETTTINGS, &aoe.global_settings, ImGuiWindowFlags_AlwaysAutoResize)) {
				lang_current = int(lang);
				ImGui::Combo(CTXT(TextId::language), &lang_current, langs, int(LangId::max));
				lang = (LangId)genie::clamp(0, lang_current, int(LangId::max) - 1);

				static bool sfx_on, msc_on;
				sfx_on = genie::jukebox.sfx_enabled();
				msc_on = genie::jukebox.msc_enabled();

				// enable/disable audio
				if (ImGui::Checkbox(CTXT(TextId::cfg_sfx), &sfx_on))
					genie::jukebox.sfx(sfx_on);
				ImGui::SameLine();
				if (ImGui::Checkbox(CTXT(TextId::cfg_msc), &msc_on))
					genie::jukebox.msc(msc_on);

				// volume control
				static float sfx_vol = 100.0f, msc_vol = 100.0f;
				int old_vol = genie::jukebox.sfx_volume(), new_vol;

				sfx_vol = old_vol * 100.0f / genie::sfx_max_volume;
				ImGui::SliderFloat(CTXT(TextId::cfg_sfx_vol), &sfx_vol, 0, 100.0f, "%.0f");

				sfx_vol = genie::clamp(0.0f, sfx_vol, 100.0f);
				new_vol = sfx_vol * genie::sfx_max_volume / 100.0f;

				if (old_vol != new_vol)
					genie::jukebox.sfx_volume(new_vol);

				old_vol = genie::jukebox.msc_volume();
				msc_vol = old_vol * 100.0f / genie::sfx_max_volume;
				ImGui::SliderFloat(CTXT(TextId::cfg_msc_vol), &msc_vol, 0, 100.0f, "%.0f");

				msc_vol = genie::clamp(0.0f, msc_vol, 100.0f);
				new_vol = msc_vol * genie::sfx_max_volume / 100.0f;

				if (old_vol != new_vol)
					genie::jukebox.msc_volume(new_vol);

				if (ImGui::Checkbox(CTXT(TextId::cfg_fullscreen), &video.fullscreen))
					video.set_fullscreen(video.fullscreen);

				if (ImGui::Button(CTXT(TextId::cfg_center_window)))
					video.center();

				str_dim_lgy[3] = CTXT(TextId::dim_custom);

				if (ImGui::Combo(CTXT(TextId::cfg_display_mode), &video.mode, str_dim_lgy, IM_ARRAYSIZE(str_dim_lgy))) {
					if (video.mode < 3)
						SDL_SetWindowSize(window, dim_lgy[video.mode].w, dim_lgy[video.mode].h);
				}

				ImGui::EndPopup();
			}

			if (aoe.wip) {
				if (!was_wip)
					ImGui::OpenPopup(MODAL_WIP);

				if (ImGui::BeginPopupModal(MODAL_WIP, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::TextUnformatted(CTXT(TextId::wip));
					ImGui::EndPopup();
				}
			}

			if (aoe.working) {
				if (!was_working)
					ImGui::OpenPopup(MODAL_LOADING);

				if (p.total && ImGui::BeginPopupModal(MODAL_LOADING, NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
					ImGui::TextUnformatted(p.desc.c_str());
					ImGui::ProgressBar(float(p.step) / p.total);
					ImGui::EndPopup();
				}
			}

			was_working = aoe.working;
			was_global_settings = aoe.global_settings;
			was_wip = aoe.wip;

			// Rendering
			ImGui::Render();
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// do our stuff
			aoe.display();

			// do imgui stuff
			//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
			FrameMark
			SDL_GL_SwapWindow(window);
		}

		genie::settings.load(video);
	}

	// try to stop worker
	// for whatever reason, it may be busy for a long time (e.g. blocking socket call)
	// in that case, wait up to 3 seconds
	w_bkg.tasks.stop();

	genie::settings.save();

	auto future = std::async(std::launch::async, &std::thread::join, &t_bkg);
	if (future.wait_for(std::chrono::seconds(3)) == std::future_status::timeout)
		_Exit(0); // worker still busy, dirty exit


	// Cleanup
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
