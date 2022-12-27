#include "assets.hpp"

#include "../async.hpp"
#include "../legacy.hpp"
#include "../engine.hpp"

#include <algorithm>

namespace aoe {

using namespace io;

Background::Background() : drs(), pal(nullptr, SDL_FreePalette), img() {}

void Background::load(DRS &drs, DrsId id) {
	this->drs = DrsBkg(drs.open_bkg(id));
	pal = drs.open_pal((DrsId)this->drs.pal_id);
	auto slp = drs.open_slp((DrsId)this->drs.bkg_id[2]);
	img.load(pal.get(), slp, 0, 0);
}

Assets::Assets(int id, Engine &eng, const std::string &path)
	: path(path)
	, bkg_main(), bkg_multiplayer()
	, bkg_tex(nullptr, SDL_FreeSurface)
{
	// TODO use engine view to prevent crash when closed while ctor is still running
	UI_TaskInfo info(eng.ui_async("Verifying game data", "Locating interface data", id, 4));

	DRS drs_ui(path + "/data/Interfac.drs");

	info.next("Loading interface data");

	bkg_main.load(drs_ui, DrsId::bkg_main_menu);
	bkg_multiplayer.load(drs_ui, DrsId::bkg_multiplayer);

	drs_ui.open_bkg(DrsId::bkg_achievements);
	drs_ui.open_bkg(DrsId::bkg_defeat);

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

}
