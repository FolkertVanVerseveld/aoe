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
#include "imgui/ImGuiFileDialog.h"

#include "prodcons.hpp"
#include "lang.hpp"
#include "drs.hpp"
#include "math.hpp"
#include "audio.hpp"
#include "graphics.hpp"

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

#include <signal.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_mixer.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define FDLG_CHOOSE_DIR "Fdlg Choose AoE dir"
#define MSG_INIT "Msg Init"

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
static std::atomic_bool running(true);

enum class MenuId {
	config,
	start,
	editor,
};

MenuId menu_id = MenuId::config;

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

namespace genie {

Assets assets;

}

enum class GLcmdType {
	stop,
	set_bkg,
};

struct GLcmd final {
	GLcmdType type;
	std::variant<std::nullopt_t, genie::Tilesheet> data;

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

class Worker final {
public:
	ConcurrentChannel<WorkTask> tasks; // in
	ConcurrentChannel<WorkResult> results; // out
	GLchannel &ch;

	std::mutex mut;
	WorkerProgress p;

	Worker(GLchannel &ch) : tasks(WorkTaskType::stop, 10), results(WorkResult{ WorkTaskType::stop, WorkResultType::stop }, 10), ch(ch), mut(), p() {}

	int run() {
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

	bool progress(WorkerProgress &p) {
		std::unique_lock<std::mutex> lock(mut);

		unsigned v = this->p.step.load();

		p.desc = this->p.desc;
		p.step.store(v);
		p.total = this->p.total;

		return v != p.total;
	}

private:
	void start(unsigned total, unsigned step=0) {
		std::unique_lock<std::mutex> lock(mut);
		if (!running)
			throw WorkInterrupt();
		p.step = step;
		p.total = total;
	}

	void set_desc(const std::string &s) {
		std::unique_lock<std::mutex> lock(mut);
		if (!running)
			throw WorkInterrupt();
		p.desc = s;
	}

	template<typename... Args>
	void done(Args&&... args) {
		std::unique_lock<std::mutex> lock(mut);
		p.step = p.total;
		results.produce(args...);
	}

	void load_menu(WorkTask &task) {
		MenuId id = std::get<MenuId>(task.data);

		start(3);
		set_desc("Loading menu dialog");

		// find corresponding dialog
		const genie::res_id dlg_ids[] = {
			-1,
			(unsigned)genie::DrsId::menu_start,
			(unsigned)genie::DrsId::menu_editor,
		};
		genie::Dialog dlg(genie::assets.blobs[2]->open_dlg(dlg_ids[(unsigned)id]));
		++p.step;

		genie::TilesheetBuilder bld;

		// add all backgrounds
		for (int i = 0; i < 3; ++i) {
			dlg.set_bkg(i);
			genie::Animation &anim = *dlg.bkganim.get();
			bld.emplace(anim.subimage(0), dlg.pal.get());
		}

		// send back to main thread
		genie::Tilesheet ts(bld);
		ch.cmds.produce(GLcmdType::set_bkg, ts);

		++p.step;
		set_desc("Loading menu music");

		switch (id) {
			case MenuId::start: {
				genie::jukebox.play(genie::MusicId::open);
				++p.step;
				done(task, WorkResultType::success);
				break;
			}
			default:
				done(task, WorkResultType::fail);
				break;
		}
	}

	void check_root(const FdlgItem &item) {
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

		start(2);
		set_desc(TXT(TextId::work_drs));

		std::string dir(item.first);

		try {
			for (unsigned i = 0; i < COUNT; ++i) {
				std::string path(genie::drs_path(item.first, fnames[i]));
				if ((fd[i] = genie::open(path.c_str())) == genie::fd_invalid) {
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
					std::string path(genie::drs_path(item.second, fnames[i]));
					if ((fd[i] = genie::open(path.c_str())) == genie::fd_invalid) {
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
		genie::game_dir = dir;
		done(WorkTaskType::check_root, WorkResultType::success, TXT(TextId::work_drs_success));
#undef COUNT
	}
};

static void worker_init(Worker &w, int &status) {
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
	std::string alt;

	Preview() : tex(0), bnds(), alt() {
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
		if (!bnds.w || !bnds.h || !alt.empty())
			ImGui::TextUnformatted(alt.empty() ? "(no image data)" : alt.c_str());
		else
			ImGui::Image((ImTextureID)tex, ImVec2(bnds.w, bnds.h));
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
		, channel(-1), dialog_mode(0), lookup_id(0), looping(true), autoplay(true), preview_changed(true)
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
							ImGui::Combo("Display mode", &dialog_mode, str_dim_lgy, IM_ARRAYSIZE(str_dim_lgy));

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

class VideoControl final {
public:
	int displays, display, mode;
	bool aspect, legacy, fullscreen;
	std::vector<SDL_Rect> bounds;
	SDL_Rect size, winsize;

	VideoControl() : displays(SDL_GetNumVideoDisplays()), display(0), mode(3), aspect(true), legacy(true), fullscreen(false), bounds(displays > 0 ? displays : 0), size(), winsize({0, 0, 800, 600}) {
		for (int i = 0; i < displays; ++i)
			SDL_GetDisplayBounds(i, &bounds[i]);
	}

	void idle() {
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

	void center() {
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

	void set_fullscreen(bool enable) {
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

	void restore(GlobalConfiguration &cfg);
};

static constexpr const char *defcfg = "settings.dat";
static constexpr uint32_t cfghdr = 0x1b28d7a4; // Nothing special, just smashing some keys

/**
 * Application-wide user-settings save/restore wrapper.
 * Currently only looks at current working directory for settings.dat.
 * Any errors are ignored.
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
 */
static class GlobalConfiguration final {
public:
	// video
	int display_index, mode;
	SDL_Rect display, scrbnds;
	bool fullscreen;
	// audio
	bool msc_on, sfx_on;
	int msc_vol, sfx_vol;

	void load(const VideoControl &video) {
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

	bool load() { return load(defcfg); }

	bool load(const char *fname) {
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

			this->display = display;
			this->scrbnds = scrbnds;
			this->display_index = display_index;
			this->mode = mode;
			this->fullscreen = fullscreen;

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

	void save() { save(defcfg); }

	void save(const char *fname) {
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
		} catch (std::ios_base::failure &f) {
			fprintf(stderr, "%s: %s\n", fname, f.what());
		}
	}
} settings;

void VideoControl::restore(GlobalConfiguration &cfg) {
	// only apply configuration if display settings match
	bool match = false;

	for (SDL_Rect b : bounds)
		if (b.x == cfg.display.x && b.y == cfg.display.y && b.w == cfg.display.w && b.h == cfg.display.h) {
			match = true;
			break;
		}

	if (!match)
		return;

	SDL_SetWindowPosition(window, cfg.scrbnds.x, cfg.scrbnds.y);
	SDL_SetWindowSize(window, cfg.scrbnds.w, cfg.scrbnds.h);

	set_fullscreen(cfg.fullscreen);
}

static void display(VideoControl &video, genie::Texture &tex_bkg) {
	SDL_Rect bnds;

	SDL_GetWindowSize(window, &bnds.w, &bnds.h);
	SDL_GetWindowPosition(window, &bnds.x, &bnds.y);

	video.size = bnds;

	int lgy_index = 2;

	// prevent division by zero
	if (bnds.h < 1) bnds.h = 1;

	long long need_w = bnds.w, need_h = bnds.h;
	GLdouble left = 0, top = 0, right = 1024, bottom = 1024;

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

		glViewport(bnds.x, bnds.y, need_w, need_h);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (video.legacy) {
		left = dim_lgy[lgy_index].x;
		top = dim_lgy[lgy_index].y;
		right = left + dim_lgy[lgy_index].w;
		bottom = top + dim_lgy[lgy_index].h;
	}
	glOrtho(left, right, bottom, top, -1, 1);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glColor3f(1.0f, 1.0f, 1.0f);
	glEnable(GL_TEXTURE_2D);

	if (menu_id != MenuId::config && tex_bkg.tex) {
		glBindTexture(GL_TEXTURE_2D, tex_bkg.tex);

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
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesVietnamese());

	// Our state
	bool show_demo_window = true;
	bool ther_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	auto fd = igfd::ImGuiFileDialog::Instance();

	bool fs_choose_root = false, auto_detect = false;
	GLchannel glch;
	genie::Texture tex_bkg;
	Worker w_bkg(glch);
	int err_bkg;

	std::thread t_bkg(worker_init, std::ref(w_bkg), std::ref(err_bkg));
	WorkerProgress p = { 0 };
	std::string bkg_result;

	VideoControl video;
	settings.load(video);

	if (settings.load()) {
		video.restore(settings);
	}

	// some stuff needs to be cleaned up before we shut down, so we need another scope here
	{
		std::stack<const char*> auto_paths;
#if windows
		auto_paths.emplace("C:\\Program Files (x86)\\Microsoft Games\\Age of Empires");
		auto_paths.emplace("C:\\Program Files\\Microsoft Games\\Age of Empires");
#else
		auto_paths.emplace("/media/cdrom");
#endif

		if (!auto_paths.empty()) {
			auto_detect = true;
			std::string path(auto_paths.top());
			w_bkg.tasks.produce(WorkTaskType::check_root, std::make_pair(path, path));
			auto_paths.pop();
		}

		DrsView drsview;

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
						if (event.key.keysym.sym == SDLK_BACKQUOTE)
							show_debug = !show_debug;
						break;
					}
				}
			}

			video.idle();

			// Start the Dear ImGui frame
			ImGui_ImplOpenGL2_NewFrame();
			ImGui_ImplSDL2_NewFrame(window);
			ImGui::NewFrame();

			// disable saving imgui.ini
			io.IniFilename = NULL;

			if (show_debug) {
				// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
				if (show_demo_window)
					ImGui::ShowDemoWindow(&show_demo_window);

				if (ImGui::Begin("Debug control")) {
					ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
								w_bkg.tasks.produce(WorkTaskType::play_music, id);
							}

							if (ImGui::Button("Panic")) {
								genie::jukebox.stop(-1);
								genie::jukebox.stop();
							}

							ImGui::EndTabItem();
						}
						if (ImGui::BeginTabItem("Video")) {
							ImGui::Text("Window size: %.0fx%.0f\n", io.DisplaySize.x, io.DisplaySize.y);
							ImGui::Checkbox("Keep aspect ratio", &video.aspect);

							if (ImGui::Checkbox("Fullscreen", &video.fullscreen))
								video.set_fullscreen(video.fullscreen);

							if (ImGui::Button("Center"))
								video.center();

							if (video.displays < 1) {
								ImGui::TextUnformatted("Could not get display info");
							} else {
								ImGui::Text("Display count: %d\n", video.displays);

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
			}

			// munch all OpenGL commands
			for (std::optional<GLcmd> cmd; cmd = glch.cmds.try_consume(), cmd.has_value();) {
				switch (cmd->type) {
					case GLcmdType::stop:
						break;
					case GLcmdType::set_bkg:
						tex_bkg.ts = std::move(std::get<genie::Tilesheet>(cmd->data));
						tex_bkg.ts.write(tex_bkg.tex);
						break;
					default:
						assert("bad opengl command" == 0);
						break;
				}
			}

			static int lang_current = 0;
			static bool show_about = false;
			bool working = w_bkg.progress(p);

			// munch all results
			for (std::optional<WorkResult> res; res = w_bkg.results.try_consume(), res.has_value();) {
				switch (res->type) {
					case WorkResultType::success:
						switch (res->task.type) {
							case WorkTaskType::stop:
								break;
							case WorkTaskType::check_root:
								fs_has_root = true;
								// start game automatically on auto detect
								if (auto_detect) {
									auto_detect = false;
									w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::start);
								}
								break;
							case WorkTaskType::load_menu:
								menu_id = std::get<MenuId>(res->task.data);
								break;
							default:
								assert("bad task type" == 0);
								break;
						}
						break;
					case WorkResultType::stop:
						break;
					case WorkResultType::fail:
						if (!auto_paths.empty()) {
							std::string path(auto_paths.top());
							w_bkg.tasks.produce(WorkTaskType::check_root, std::make_pair(path, path));
							auto_paths.pop();
							working = true;
						} else {
							auto_detect = false;
						}
						break;
					default:
						assert("bad result type" == 0);
						break;
				}
				bkg_result = res->what;
			}

			switch (menu_id) {
				case MenuId::config:
					ImGui::Begin("Startup configuration", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
					{
						ImGui::SetWindowSize(io.DisplaySize);
						ImGui::SetWindowPos(ImVec2());

						if (ImGui::Button(TXT(TextId::btn_about)))
							show_about = true;

						ImGui::TextWrapped(TXT(TextId::hello));

						lang_current = int(lang);
						ImGui::Combo(TXT(TextId::language), &lang_current, langs, int(LangId::max));
						lang = (LangId)genie::clamp(0, lang_current, int(LangId::max) - 1);

						if (!working && ImGui::Button(TXT(TextId::set_game_dir)))
							fd->OpenDialog(FDLG_CHOOSE_DIR, TXT(TextId::dlg_game_dir), 0, ".");

						if (fd->FileDialog(FDLG_CHOOSE_DIR, ImGuiWindowFlags_NoCollapse, ImVec2(400, 200)) && !working) {
							if (fd->IsOk == true) {
								std::string fname(fd->GetFilepathName()), path(fd->GetCurrentPath());
								printf("fname = %s\npath = %s\n", fname.c_str(), path.c_str());

								fs_has_root = false;
								w_bkg.tasks.produce(WorkTaskType::check_root, std::make_pair(fname, path));
							}

							fd->CloseDialog(FDLG_CHOOSE_DIR);
						}

						if (working && p.total) {
							ImGui::TextUnformatted(p.desc.c_str());
							ImGui::ProgressBar(float(p.step) / p.total);
						}

						if (!bkg_result.empty())
							ImGui::TextUnformatted(bkg_result.c_str());

						if (ImGui::Button(TXT(TextId::btn_quit)))
							running = false;

						if (fs_has_root) {
							ImGui::SameLine();
							if (ImGui::Button(TXT(TextId::btn_startup)))
								w_bkg.tasks.produce(WorkTaskType::load_menu, MenuId::start);
						}
					}
					ImGui::End();
					break;
				case MenuId::start:
					ImGui::Begin("Main menu placeholder");
					{
						if (ImGui::Button(TXT(TextId::btn_about)))
							show_about = true;

						if (ImGui::Button(TXT(TextId::btn_editor)))
							menu_id = MenuId::editor;

						if (ImGui::Button(TXT(TextId::btn_quit)))
							running = false;

						lang_current = int(lang);
						ImGui::Combo(TXT(TextId::language), &lang_current, langs, int(LangId::max));
						lang = (LangId)genie::clamp(0, lang_current, int(LangId::max) - 1);
					}
					ImGui::End();
					break;
				case MenuId::editor:
					ImGui::Begin("Editor menu placeholder");
					{
						if (ImGui::Button(TXT(TextId::btn_back)))
							menu_id = MenuId::start;
					}
					ImGui::End();
					break;
			}

			if (show_about) {
				ImGui::Begin("About", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
				ImGui::SetWindowSize(io.DisplaySize);
				ImGui::SetWindowPos(ImVec2());
				ImGui::TextWrapped("Copyright 2016-%d Folkert van Verseveld\n", year_start);
				ImGui::TextWrapped(TXT(TextId::about));
				ImGui::TextWrapped("Using OpenGL version: %d.%d", major, minor);

				if (ImGui::Button(TXT(TextId::btn_back)))
					show_about = false;
				ImGui::End();
			}

			// Rendering
			ImGui::Render();
			glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
			glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
			glClear(GL_COLOR_BUFFER_BIT);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// do our stuff
			display(video, tex_bkg);

			// do imgui stuff
			//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
			ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
			SDL_GL_SwapWindow(window);
		}

		settings.load(video);
	}

	// try to stop worker
	// for whatever reason, it may be busy for a long time (e.g. blocking socket call)
	// in that case, wait up to 3 seconds
	w_bkg.tasks.stop();

	settings.save();

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
