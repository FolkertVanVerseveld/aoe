#include "assets.hpp"

#include "../legacy.hpp"
#include "../engine.hpp"

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
			images[p * MAX_PLAYERS + i].load(pal, slp, i, p);
}

Image &Animation::subimage(unsigned index, unsigned player) {
	return dynamic ? images[(player % MAX_PLAYERS) * image_count + index % image_count] : images[index % image_count];
}

Assets::Assets(int id, Engine &eng, const std::string &path)
	: drs_gifs(), path(path), drs_ids(), bkg_cols(), ts_ui(), gif_cursors()
{
	ZoneScoped;
	// TODO use engine view to prevent crash when closed while ctor is still running
	UI_TaskInfo info(eng.ui_async("Verifying game data", "Loading interface data", id, 5));

	load_gfx(eng, info);
	load_audio(eng, info);
}

void Assets::load_gfx(Engine &eng, UI_TaskInfo &info) {
	ZoneScoped;

	DRS drs_ui(path + "/data/Interfac.drs");

	Background bkg_main, bkg_singleplayer, bkg_multiplayer, bkg_editor, bkg_victory, bkg_defeat, bkg_mission, bkg_achievements;

	{
		ZoneScopedN("load backgrounds");
		bkg_main.load(drs_ui, DrsId::bkg_main_menu); bkg_cols[DrsId::bkg_main_menu] = bkg_main.cols;
		bkg_singleplayer.load(drs_ui, DrsId::bkg_singleplayer); bkg_cols[DrsId::bkg_singleplayer] = bkg_singleplayer.cols;
		bkg_multiplayer.load(drs_ui, DrsId::bkg_multiplayer); bkg_cols[DrsId::bkg_multiplayer] = bkg_multiplayer.cols;
		bkg_editor.load(drs_ui, DrsId::bkg_editor); bkg_cols[DrsId::bkg_editor] = bkg_editor.cols;
		bkg_victory.load(drs_ui, DrsId::bkg_victory); bkg_cols[DrsId::bkg_victory] = bkg_victory.cols;
		bkg_defeat.load(drs_ui, DrsId::bkg_defeat); bkg_cols[DrsId::bkg_defeat] = bkg_defeat.cols;
		bkg_mission.load(drs_ui, DrsId::bkg_mission); bkg_cols[DrsId::bkg_mission] = bkg_mission.cols;
		bkg_achievements.load(drs_ui, DrsId::bkg_achievements); bkg_cols[DrsId::bkg_achievements] = bkg_achievements.cols;
	}

	{
		ZoneScopedN("load game graphics");
		auto pal = drs_ui.open_pal(DrsId::pal_default);
		gif_cursors.load(drs_ui, pal.get(), DrsId::gif_cursors);
	}

	load_terrain(eng, info);

	info.next("Packing graphics");
	{
		ZoneScopedN("pack");
		gfx::ImagePacker p;

		// register images for packer
		drs_ids[DrsId::bkg_main_menu] = p.add_img(bkg_main);
		drs_ids[DrsId::bkg_singleplayer] = p.add_img(bkg_singleplayer);
		drs_ids[DrsId::bkg_multiplayer] = p.add_img(bkg_multiplayer);
		drs_ids[DrsId::bkg_editor] = p.add_img(bkg_editor);
		drs_ids[DrsId::bkg_victory] = p.add_img(bkg_victory);
		drs_ids[DrsId::bkg_defeat] = p.add_img(bkg_defeat);
		drs_ids[DrsId::bkg_mission] = p.add_img(bkg_mission);
		drs_ids[DrsId::bkg_achievements] = p.add_img(bkg_achievements);

		// START extract
		ImageSet gifs;

		for (unsigned i = 0; i < gif_cursors.all_count; ++i)
			gifs.imgs.emplace_back(p.add_img(gif_cursors.images[i].surface.get()));

		drs_gifs[DrsId::gif_cursors] = gifs;
		// STOP extract

		// pack images
		GLint size = eng.gl().max_texture_size;
		ts_ui = p.collect(size, size);
	}
}

void Assets::load_terrain(Engine &eng, UI_TaskInfo &info) {
	ZoneScoped;
	info.next("Loading terrain data");

	DRS drs_terrain(path + "/data/Terrain.drs");

	auto slp = drs_terrain.open_slp(io::DrsId::trn_desert);
	printf("desert tiles: %llu\n", (unsigned long long)slp.frames.size());
	slp = drs_terrain.open_slp(io::DrsId::trn_grass);
	printf("grass tiles: %llu\n", (unsigned long long)slp.frames.size());
	slp = drs_terrain.open_slp(io::DrsId::trn_water);
	printf("water tiles: %llu\n", (unsigned long long)slp.frames.size());
	slp = drs_terrain.open_slp(io::DrsId::trn_deepwater);
	printf("deep water tiles: %llu\n", (unsigned long long)slp.frames.size());
}

void Assets::load_audio(Engine &eng, UI_TaskInfo &info) {
	ZoneScoped;
	info.next("Load chat audio");

	eng.sfx.reset();

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
}

const ImageRef &Assets::at(DrsId id) const {
	ImageRef r(drs_ids.at(id), SDL_Rect{ 0, 0, 0, 0 });

	auto it = ts_ui.imgs.find(r);

	if (it == ts_ui.imgs.end())
		throw std::runtime_error("bad id");

	return *it;
}

}
