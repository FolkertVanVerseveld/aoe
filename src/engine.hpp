#pragma once

#include <cassert>

#include "ui/hud.hpp"
#include "prodcons.hpp"
#include "geom.hpp"

#include <SDL2/SDL.h>

namespace genie {

enum AnimationId {
	desert,
	grass,
	water_shallow,
	water_deep,
	corner_water_desert,
	corner_desert_grass,
	overlay_water,
	ui_buttons,
};

enum class MenuId {
	config,
	start,
	singleplayer,
	editor,
	scn_edit,
};

class GlobalConfiguration;

class VideoControl final {
public:
	int displays, display, mode;
	bool aspect, legacy, fullscreen;
	std::vector<SDL_Rect> bounds;
	SDL_Rect size, winsize, vp;

	VideoControl();

	void motion(double &px, double &py, int x, int y);

	void idle();
	void center();
	void set_fullscreen(bool enable);

	void restore(GlobalConfiguration &cfg);
};

/**
* Application-wide user-settings save/restore wrapper.
* Currently only looks at current working directory for settings.dat.
* Any errors are ignored.
* TODO add custom installation path
*
* File format:
* 00-03  magic identifier
* 04     display index
* 05     mode
* 06     flags: 0x01 fullscreen, 0x02 music enabled, 0x04 sound enabled
* 07     reserved
* 18-27  display.x, display.y, display.w, display.h  display boundaries
* 28-37  scrbnds.x, scrbnds.y, scrbnds.w, scrbnds.h  application boundaries
* 38-39  music volume
* 3a-3b  sound volume
* 3c-3d  last valid game path length
* 3e-XX  last valid game path string data
*/
class GlobalConfiguration final {
public:
	// video
	int display_index, mode;
	SDL_Rect display, scrbnds;
	bool fullscreen;
	// audio
	bool msc_on, sfx_on;
	int msc_vol, sfx_vol;

	std::string gamepath;

	GlobalConfiguration() = default;

	void load(const VideoControl &video);

	bool load();
	bool load(const char *fname);

	void save();
	void save(const char *fname);
};

extern GlobalConfiguration settings;

enum class GLcmdType {
	stop,
	set_bkg,
	set_dlgcol,
	set_anims,
};

struct GLcmd final {
	GLcmdType type;
	std::variant<std::nullopt_t, genie::Tilesheet, genie::DialogColors, std::map<AnimationId, unsigned>> data;

	GLcmd(GLcmdType type) : type(type), data(std::nullopt) {}

	template<typename... Args>
	GLcmd(GLcmdType type, Args&&... args) : type(type), data(args...) {}
};

enum class GLcmdResultType {
	stop,
	error,
};

struct GLcmdResult final {
	GLcmdResultType type;

	GLcmdResult(GLcmdResultType type) : type(type) {}
};

/** OpenGL wrapper such that our main worker call do OpenGL stuff while running on another thread. */
class GLchannel final {
public:
	ConcurrentChannel<GLcmd> cmds;
	ConcurrentChannel<GLcmdResult> results;

	GLchannel() : cmds(GLcmdType::stop, 10), results(GLcmdResultType::stop, 10) {}
};

enum class WorkTaskType {
	stop,
	check_root,
	load_menu,
	play_music,
};

using FdlgItem = std::pair<std::string, std::string>;

class WorkTask final {
public:
	WorkTaskType type;
	std::variant<std::nullopt_t, FdlgItem, MenuId, genie::MusicId> data;

	WorkTask(WorkTaskType t) : type(t), data(std::nullopt) {}

	template<typename... Args>
	WorkTask(WorkTaskType t, Args&&... args) : type(t), data(args...) {}
};

enum class WorkResultType {
	stop,
	success,
	fail,
};

class WorkResult final {
public:
	WorkTask task;
	WorkResultType type;
	std::string what;

	WorkResult(const WorkTask &task, WorkResultType rt) : task(task), type(rt), what() {}
	WorkResult(const WorkTask &task, WorkResultType rt, const std::string &what) : task(task), type(rt), what(what) {}
};

struct WorkerProgress final {
	std::atomic<unsigned> step;
	unsigned total;
	std::string desc;
};

// trick to abort worker thread on shutdown
struct WorkInterrupt final {
	WorkInterrupt() {}
};

class Worker final {
public:
	ConcurrentChannel<WorkTask> tasks; // in
	ConcurrentChannel<WorkResult> results; // out
	GLchannel &ch;

	std::mutex mut;
	WorkerProgress p;

	Worker(GLchannel &ch);

	int run();
	bool progress(WorkerProgress &p);
private:
	void start(unsigned total, unsigned step=0);

	void set_desc(const std::string &s);

	template<typename... Args>
	void done(Args&&... args) {
		std::unique_lock<std::mutex> lock(mut);
		p.step = p.total;
		results.produce(args...);
	}

	void load_menu(WorkTask &task);

	void check_root(const FdlgItem &item);
};

/** Immovable object that cannot change state while a scenario is running. */
class StaticObject final {
public:
	genie::Box<double> bounds; // screen position
	size_t tpos;
	int8_t height;
	uint8_t id;
	unsigned image_index; // if tpos != (size_t)-1 then subimage else animation index (e.g. debris)

	StaticObject() : bounds(), tpos((size_t)-1), height(0), id(1), image_index((unsigned)-1) {}
	StaticObject(const genie::Box<double> &bounds, size_t tpos) : bounds(bounds), tpos(tpos), height(0), id(1), image_index(0) {}
	StaticObject(const genie::Box<double> &bounds, unsigned anim_index) : bounds(bounds), tpos((size_t)-1), height(0), id(1), image_index(anim_index) {}
};

static genie::Box<double> getStaticObjectAABB(const StaticObject &obj) {
	return obj.bounds;
}

static bool cmpStaticObject(const StaticObject &lhs, const StaticObject &rhs) {
	assert(lhs.tpos != (size_t)-1);
	return lhs.tpos == rhs.tpos;
}

struct TileInfo final {
	genie::AnimationId id;
	unsigned subimage;

	TileInfo(genie::AnimationId id, unsigned subimage) : id(id), subimage(subimage) {}
};

class Terrain final {
public:
	// duplicate information as static_objects also has this, but this eases tile info lookup
	std::vector<uint8_t> tiles, subimg, overlay;
	std::vector<int8_t> hmap;
	int w, h;

	int left, right, bottom, top;
	long long tile_focus;
	StaticObject *tile_focus_obj;

	static constexpr int tw = 64, th = 32, thw = tw / 2, thh = th / 2;

	std::vector<TileInfo> tiledata;

	// data for unproject
	GLfloat proj[16], mv[16];
	GLint vp[4];

	genie::Box<double> cam;
	bool cam_moved;
	// FIXME /int/float/
	// images may have odd width or height dimensions, which breaks drawing and collision detection...
	genie::Quadtree<StaticObject, decltype(getStaticObjectAABB), decltype(cmpStaticObject), double> static_objects;
	std::vector<const StaticObject*> vis_static_objects;
	float cull_margin;

	genie::Texture *tex;
	MemoryEditor memedit;
	std::string filename;
	std::vector<uint8_t> filedata;
	std::unique_ptr<genie::io::Scenario> scn;

	Terrain();
private:
	void build_tiles(genie::Texture &tex);
public:
	void load();

	void init(genie::Texture &tex);
	void invalidate_cam_focus(std::pair<StaticObject*, bool> ret, unsigned image_index, uint8_t id, int8_t height, bool update_focus);

	void update_tile(const StaticObject &obj);
	void resize(int w, int h);
	void select(int mx, int my, int vpx, int vpy);

	void set_view();

	/* Draw terrain. Assumes glBegin hasn't been called yet and the texture is bound. */
	void show(genie::Texture &tex);
};

class Player final {
public:
	unsigned id, civ_id;
	std::string name;

	Player(unsigned id, unsigned civ_id, const std::string &name) : id(id), civ_id(civ_id), name(name) {}
};

class VirtualPlayer final {
public:
	unsigned id, p_id;
	std::string name;

	VirtualPlayer(unsigned id, unsigned p_id, const std::string &name) : id(id), p_id(p_id), name(name) {}
};

class Team final {
public:
	std::string name;
};

class AoE final {
public:
	VideoControl video;
	genie::Texture tex_bkg;
	genie::Tilesheet ts_next;
	genie::DialogColors dlgcol;
	std::map<AnimationId, unsigned> anims;
	Hud hud;
	int mouse_x, mouse_y;
	bool working, global_settings, wip;
	Worker &w_bkg;
	Terrain terrain;
	std::vector<Team> teams;
	std::vector<Player> players;
	std::vector<VirtualPlayer> vplayers;

	// singleplayer menu state vars
	unsigned player_count; // 0 means 1 player!!

	int lgy_index;
	// orthogonal viewport
	GLdouble left, right, top, bottom;

	float cam_x, cam_y, cam_zoom;

	bool show_drs, open_dlg;
	const char *dlg_id;

	static constexpr float zoom_min = 0.01f, zoom_max = 5.0f;

	AoE(Worker &w_bkg);

	void load_menu(MenuId id);
	void idle(SDL_Event &event);

	void draw_background();
	void draw_terrain();
	void display();
	void display_editor(SDL_Rect bnds);
	void display_scn_edit(SDL_Rect bnds);
	void display_start(SDL_Rect bnds);
	void display_singleplayer(SDL_Rect bnds);
	void display_singleplayer_menu(SDL_Rect bnds);
	void display_singleplayer_game(SDL_Rect bnds);
};

}
