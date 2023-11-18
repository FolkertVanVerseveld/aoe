#include "../ui.hpp"
#include "../legacy/scenario.hpp"
#include "../game.hpp"

namespace aoe {

namespace ui {

void UICache::set_scn(io::Scenario &scn) {
	ScenarioSettings settings;

	settings.players.resize(scn.players);

	// TODO fill player data

	settings.width = scn.w;
	settings.height = scn.h;

	scn_game.resize(settings);
	scn_game.terrain_set(scn.tile_types, scn.tile_height, 0, 0, scn.w, scn.h);

	for (const auto kv : scn.entities)
		scn_game.entity_add(kv.second);
}

}

}
