#include "../../ui.hpp"
#include "../../world/game/game_settings.hpp"

#define POPULATION_STEP 5
#define RESOURCES_STEP 10
#define MAPSIZE_STEP 4

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

	f.scalar("Width", scn.width, MAPSIZE_STEP, Terrain::min_size, Terrain::max_size);
	if (scn.square)
		scn.height = scn.width;
	f.scalar("Height", scn.height, MAPSIZE_STEP, Terrain::min_size, Terrain::max_size);
	if (scn.square)
		scn.width = scn.height;

	f.chkbox("Fixed position", scn.fixed_start);
	f.chkbox("Reveal map", scn.explored);
	f.chkbox("Full Tech Tree", scn.all_technologies);
	f.chkbox("Enable cheating", scn.cheating);

	f.combo("Age", scn.age, old_lang.age_names);
	f.scalar("Max pop.", scn.popcap, POPULATION_STEP, min_popcap, max_popcap);

	f.str("Resources:");
	f.scalar("food", scn.res.food, RESOURCES_STEP, 0, max_resource_value);
	f.scalar("wood", scn.res.wood, RESOURCES_STEP, 0, max_resource_value);
	f.scalar("gold", scn.res.gold, RESOURCES_STEP, 0, max_resource_value);
	f.scalar("stone", scn.res.stone, RESOURCES_STEP, 0, max_resource_value);

	f.scalar("villagers", scn.villagers, 1, min_villagers, max_villagers);
}

}
}