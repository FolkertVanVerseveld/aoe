#include "assets.hpp"

#include "../async.hpp"
#include "../legacy.hpp"
#include "../engine.hpp"

namespace aoe {

using namespace io;

Assets::Assets(int id, Engine &eng, const std::string &path) {
	// TODO use engine view to prevent crash when closed while ctor is still running
	UI_TaskInfo info(eng.ui_async("Verifying game data", "Locating interface data", id, 4));

	DRS drs_ui(path + "/data/Interfac.drs");

	info.next("Loading interface data");
	DrsBkg bkg(drs_ui.open_bkg(DrsId::bkg_main_menu));
	drs_ui.open_pal((DrsId)bkg.pal_id);
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

	DRS drs_sounds(path + "/data/sounds.drs");

	info.next("Load game audio");

	eng.sfx.load_sfx(SfxId::sfx_ui_click, drs_sounds.open_wav(DrsId::sfx_ui_click));
	eng.sfx.load_taunt(TauntId::max, drs_sounds.open_wav(DrsId::sfx_priest_convert2));
}

}
