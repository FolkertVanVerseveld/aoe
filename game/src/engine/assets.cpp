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

	Background();

	void load(io::DRS&, io::DrsId);

	operator SDL_Surface*() { return img.surface.get(); }
};

Background::Background() : drs(), pal(nullptr, SDL_FreePalette), img() {}

void Background::load(DRS &drs, DrsId id) {
	this->drs = DrsBkg(drs.open_bkg(id));
	pal = drs.open_pal((DrsId)this->drs.pal_id);
	auto slp = drs.open_slp((DrsId)this->drs.bkg_id[2]);
	img.load(pal.get(), slp, 0, 0);
}

Assets::Assets(int id, Engine &eng, const std::string &path)
	: path(path), drs_ids(), ts_ui()
{
	// TODO use engine view to prevent crash when closed while ctor is still running
	UI_TaskInfo info(eng.ui_async("Verifying game data", "Locating interface data", id, 5));

	DRS drs_ui(path + "/data/Interfac.drs");

	info.next("Loading interface data");

	Background bkg_main, bkg_multiplayer, bkg_achievements, bkg_defeat;

	bkg_main.load(drs_ui, DrsId::bkg_main_menu);
	bkg_multiplayer.load(drs_ui, DrsId::bkg_multiplayer);
	bkg_achievements.load(drs_ui, DrsId::bkg_achievements);
	bkg_defeat.load(drs_ui, DrsId::bkg_defeat);

	//drs_ui.open_bkg(DrsId::bkg_achievements);
	//drs_ui.open_bkg(DrsId::bkg_defeat);

	info.next("Pack interface graphics");

	gfx::ImagePacker p;

	// register images for packer
	drs_ids[DrsId::bkg_main_menu] = p.add_img(bkg_main);
	drs_ids[DrsId::bkg_multiplayer] = p.add_img(bkg_multiplayer);
	drs_ids[DrsId::bkg_achievements] = p.add_img(bkg_achievements);
	drs_ids[DrsId::bkg_defeat] = p.add_img(bkg_defeat);

	// pack images
	GLint size = eng.gl().max_texture_size;
	ts_ui = p.collect(size, size);

	load_audio(eng, info);
}

void Assets::load_audio(Engine &eng, UI_TaskInfo &info) {
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
