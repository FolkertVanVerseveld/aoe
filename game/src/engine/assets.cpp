#include "assets.hpp"

#include "../legacy.hpp"
#include "../engine.hpp"

#include "../legacy/strings.hpp"

#include <algorithm>

namespace aoe {

using namespace gfx;
using namespace io;

class Background final {
public:
	io::DrsBkg drs;
	std::unique_ptr<SDL_Palette, decltype(&SDL_FreePalette)> pal;
	gfx::Image img;
	BackgroundColors cols;

	Background();

	void load(io::DRS&, io::DrsId);

	operator SDL_Surface*() { return img.surface.get(); }
};

Background::Background() : drs(), pal(nullptr, SDL_FreePalette), img(), cols() {}

void Background::load(DRS &drs, DrsId id) {
	this->drs = DrsBkg(drs.open_bkg(id));
	pal = drs.open_pal((DrsId)this->drs.pal_id);
	auto slp = drs.open_slp((DrsId)this->drs.bkg_id[2]);
	img.load(pal.get(), slp, 0, 0);

	for (unsigned i = 0; i < 6; ++i)
		cols.border[i] = pal->colors[this->drs.bevel_col[i]];
}

IdPoolRef ImageSet::at(unsigned color, unsigned index) const {
	assert(dynamic);
	assert(imgs.size() % MAX_PLAYERS == 0);

	size_t image_count = imgs.size() / MAX_PLAYERS;
	return imgs.at((color % MAX_PLAYERS) * image_count + index % image_count);
}

void Animation::load(io::DRS &drs, const SDL_Palette *pal, io::DrsId id) {
	Slp slp(drs.open_slp((DrsId)id));

	images.reset(new Image[all_count = image_count = slp.frames.size()]);
	dynamic = false;

	for (unsigned i = 0; i < image_count; ++i) {
		if (images[i].load(pal, slp, i)) {
			dynamic = true;
			break;
		}
	}

	if (!dynamic)
		return;

	// restart parsing as we need one for every player
	images.reset(new Image[all_count = image_count * MAX_PLAYERS]);

	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
		for (unsigned i = 0; i < image_count; ++i)
			images[p * image_count + i].load(pal, slp, i, p);
}

Image &Animation::subimage(unsigned index, unsigned player) {
	return dynamic ? images[(player % MAX_PLAYERS) * image_count + index % image_count] : images[index % image_count];
}

Assets::Assets(int id, Engine &eng, const std::string &path)
	: drs_gifs(), path(path), drs_ids(), bkg_cols(), ts_ui(), gif_cursors(), old_lang()
{
	ZoneScoped;
	// TODO use engine view to prevent crash when closed while ctor is still running
	UI_TaskInfo info(eng.ui_async("Verifying game data", "Loading interface data", id, 7));

	eng.sfx.reset();

	load_gfx(eng, info);
	load_str(eng, info);
	load_audio(eng, info);
}

void Assets::load_gfx(Engine &eng, UI_TaskInfo &info) {
	ZoneScoped;

	DRS drs_ui(path + "/data/Interfac.drs");

	Background bkg_main, bkg_singleplayer, bkg_multiplayer, bkg_editor_menu, bkg_victory, bkg_defeat, bkg_mission, bkg_achievements;
	gfx::ImagePacker p;

	{
		ZoneScopedN("load backgrounds");
		bkg_main.load(drs_ui, DrsId::bkg_main_menu); bkg_cols[DrsId::bkg_main_menu] = bkg_main.cols;
		bkg_singleplayer.load(drs_ui, DrsId::bkg_singleplayer); bkg_cols[DrsId::bkg_singleplayer] = bkg_singleplayer.cols;
		bkg_multiplayer.load(drs_ui, DrsId::bkg_multiplayer); bkg_cols[DrsId::bkg_multiplayer] = bkg_multiplayer.cols;
		bkg_editor_menu.load(drs_ui, DrsId::bkg_editor_menu); bkg_cols[DrsId::bkg_editor_menu] = bkg_editor_menu.cols;
		bkg_victory.load(drs_ui, DrsId::bkg_victory); bkg_cols[DrsId::bkg_victory] = bkg_victory.cols;
		bkg_defeat.load(drs_ui, DrsId::bkg_defeat); bkg_cols[DrsId::bkg_defeat] = bkg_defeat.cols;
		bkg_mission.load(drs_ui, DrsId::bkg_mission); bkg_cols[DrsId::bkg_mission] = bkg_mission.cols;
		bkg_achievements.load(drs_ui, DrsId::bkg_achievements); bkg_cols[DrsId::bkg_achievements] = bkg_achievements.cols;

		// register images for packer
		drs_ids[DrsId::bkg_main_menu] = p.add_img(0, 0, bkg_main);
		drs_ids[DrsId::bkg_singleplayer] = p.add_img(0, 0, bkg_singleplayer);
		drs_ids[DrsId::bkg_multiplayer] = p.add_img(0, 0, bkg_multiplayer);
		drs_ids[DrsId::bkg_editor_menu] = p.add_img(0, 0, bkg_editor_menu);
		drs_ids[DrsId::bkg_victory] = p.add_img(0, 0, bkg_victory);
		drs_ids[DrsId::bkg_defeat] = p.add_img(0, 0, bkg_defeat);
		drs_ids[DrsId::bkg_mission] = p.add_img(0, 0, bkg_mission);
		drs_ids[DrsId::bkg_achievements] = p.add_img(0, 0, bkg_achievements);
	}

	Animation gif_menubar0;
	Image img_dialog0, img_dialog_editor;
	auto pal = drs_ui.open_pal(DrsId::pal_default);
	{
		ZoneScopedN("Loading user interface");

		gif_menubar0.load(drs_ui, pal.get(), DrsId::gif_menubar0);
		auto slp = drs_ui.open_slp(DrsId::img_dialog0);
		img_dialog0.load(pal.get(), slp, 0, 0);
		gif_cursors.load(drs_ui, pal.get(), DrsId::gif_cursors);
		slp = drs_ui.open_slp(DrsId::img_editor);
		img_dialog_editor.load(pal.get(), slp, 0, 0);
	}

	Animation trn_desert, trn_grass, trn_water, trn_deepwater;
	info.next("Loading terrain data");
	{
		ZoneScopedN("Loading terrain data");

		DRS drs_terrain(path + "/data/Terrain.drs");

		trn_desert.load(drs_terrain, pal.get(), DrsId::trn_desert);
		trn_grass.load(drs_terrain, pal.get(), DrsId::trn_grass);
		trn_water.load(drs_terrain, pal.get(), DrsId::trn_water);
		trn_deepwater.load(drs_terrain, pal.get(), DrsId::trn_deepwater);
	}

	Animation bld_town_center, bld_town_center_player, bld_barracks, bld_barracks_player;
	Animation gif_villager_stand, gif_villager_move, gif_villager_attack, gif_villager_die1, gif_villager_die2, gif_villager_decay;
	info.next("Loading game entities data");
	{
		ZoneScopedN("Loading game entities data");

		DRS drs_graphics(path + "/data/Graphics.drs");

		bld_town_center.load(drs_graphics, pal.get(), DrsId::bld_town_center);
		bld_town_center_player.load(drs_graphics, pal.get(), DrsId::bld_town_center_player);

		bld_barracks.load(drs_graphics, pal.get(), DrsId::bld_barracks);

		gif_villager_stand.load(drs_graphics, pal.get(), DrsId::gif_villager_stand);
		gif_villager_move.load(drs_graphics, pal.get(), DrsId::gif_villager_move);
		gif_villager_attack.load(drs_graphics, pal.get(), DrsId::gif_villager_attack);
		gif_villager_die1.load(drs_graphics, pal.get(), DrsId::gif_villager_die1);
		gif_villager_die2.load(drs_graphics, pal.get(), DrsId::gif_villager_die2);
		gif_villager_decay.load(drs_graphics, pal.get(), DrsId::gif_villager_decay);
	}

	info.next("Packing graphics");
	{
		ZoneScopedN("pack");

		drs_ids[DrsId::img_editor] = p.add_img(0, 0, img_dialog_editor.surface.get());

		add_gifs(p, gif_cursors, DrsId::gif_cursors);
		add_gifs(p, gif_menubar0, DrsId::gif_menubar0);

		add_gifs(p, trn_desert, DrsId::trn_desert);
		add_gifs(p, trn_grass, DrsId::trn_grass);
		add_gifs(p, trn_water, DrsId::trn_water);
		add_gifs(p, trn_deepwater, DrsId::trn_deepwater);

		add_gifs(p, bld_town_center, DrsId::bld_town_center);
		add_gifs(p, bld_town_center_player, DrsId::bld_town_center_player);

		add_gifs(p, bld_barracks, DrsId::bld_barracks);

		add_gifs(p, gif_villager_stand, DrsId::gif_villager_stand);
		add_gifs(p, gif_villager_move, DrsId::gif_villager_move);
		add_gifs(p, gif_villager_attack, DrsId::gif_villager_attack);
		add_gifs(p, gif_villager_die1, DrsId::gif_villager_die1);
		add_gifs(p, gif_villager_die2, DrsId::gif_villager_die2);
		add_gifs(p, gif_villager_decay, DrsId::gif_villager_decay);

		drs_ids[DrsId::img_dialog0] = p.add_img(0, 0, img_dialog0.surface.get());

		// pack images
		GLint size = eng.gl().max_texture_size;
		ts_ui = p.collect(size, size);
	}

	eng.sfx.load_sfx(SfxId::sfx_chat, drs_ui.open_wav(DrsId::sfx_chat));
}

void Assets::add_gifs(gfx::ImagePacker &p, Animation &a, DrsId id) {
	ImageSet gifs;

	for (unsigned i = 0; i < a.all_count; ++i) {
		Image &img = a.images[i];
		gifs.imgs.emplace_back(p.add_img(img.hotspot_x, img.hotspot_y, img.surface.get(), img.mask));
	}

	gifs.dynamic = a.dynamic;
	drs_gifs[id] = gifs;
}

void Assets::load_audio(Engine &eng, UI_TaskInfo &info) {
	ZoneScoped;
	info.next("Load chat audio");

	for (unsigned i = 0; i < (unsigned)TauntId::max; ++i) {
		char buf[8];
		snprintf(buf, sizeof buf, "%03d.wav", i + 1);

		std::string fname(path + "/sound/Taunt" + buf);
		eng.sfx.load_taunt((TauntId)i, fname.c_str());
	}

	info.next("Load game audio");

	DRS drs_sounds(path + "/data/sounds.drs");

	eng.sfx.load_sfx(SfxId::sfx_ui_click, drs_sounds.open_wav(DrsId::sfx_ui_click));
	eng.sfx.load_taunt(TauntId::max, drs_sounds.open_wav(DrsId::sfx_priest_convert2));

#define sfx(id) eng.sfx.load_sfx(SfxId:: id, drs_sounds.open_wav(DrsId:: sfx_ ## id))
	sfx(towncenter);
	sfx(barracks);
	sfx(bld_die1);
	sfx(bld_die2);
	sfx(bld_die3);
#undef sfx
}

void Assets::load_str(Engine &eng, UI_TaskInfo &info) {
	ZoneScoped;
	info.next("Load localisation data");

	// TODO change to try read and fall back to default localisation data if not found
	io::PE pe(path + "/language.dll");//"/SETUPENU.DLL");

	if ((unsigned)pe.type() < (unsigned)PE_Type::peopt)
		throw std::runtime_error("Localisation data is not a proper DLL file");

	// load strings
	old_lang.load(pe);
}

const ImageRef &Assets::at(DrsId id) const {
	ImageRef r(drs_ids.at(id), SDL_Rect{ 0, 0, 0, 0 }, NULL, 0, 0);

	auto it = ts_ui.imgs.find(r);

	if (it == ts_ui.imgs.end())
		throw std::runtime_error("bad id");

	return *it;
}

const ImageRef &Assets::at(IdPoolRef ref) const {
	ImageRef r(ref, SDL_Rect{ 0, 0, 0, 0 }, NULL, 0, 0);

	auto it = ts_ui.imgs.find(r);

	if (it == ts_ui.imgs.end())
		throw std::runtime_error("bad ref");

	return *it;
}

const ImageSet &Assets::anim_at(DrsId id) const {
	auto it = drs_gifs.find(id);

	if (it == drs_gifs.end())
		throw std::runtime_error("bad id");

	return it->second;
}

}
