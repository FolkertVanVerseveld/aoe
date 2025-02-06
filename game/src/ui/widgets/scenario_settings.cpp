#include "../../ui.hpp"
#include "../../world/game/game_settings.hpp"

namespace aoe {

namespace ui {

void show_scenario_settings(ui::Frame &f, ScenarioSettings &scn) {
	f.str("Scenario: Random map");

	f.fmt("size: %ux%u", scn.width, scn.height);
	f.scalar("seed", scn.seed);

	f.str("Size:");
	f.sl();
	f.chkbox("squared", scn.square);

	if (scn.square)
		scn.width = scn.height;

	f.sl();
	f.chkbox("wrap", scn.wrap);

	f.scalar("Width", scn.width, 1, 8, 65536);
	if (scn.square)
		scn.height = scn.width;
	f.scalar("Height", scn.height, 1, 8, 65536);
	if (scn.square)
		scn.width = scn.height;

	f.chkbox("Fixed position", scn.fixed_start);
	f.chkbox("Reveal map", scn.explored);
	f.chkbox("Full Tech Tree", scn.all_technologies);
	f.chkbox("Enable cheating", scn.cheating);

	f.combo("Age", scn.age, old_lang.age_names);
	f.scalar("Max pop.", scn.popcap);

	f.str("Resources:");
	f.scalar("food", scn.res.food, 10);
	f.scalar("wood", scn.res.wood, 10);
	f.scalar("gold", scn.res.gold, 10);
	f.scalar("stone", scn.res.stone, 10);

	f.scalar("villagers", scn.villagers, 1, 1, scn.popcap);
}

}
}