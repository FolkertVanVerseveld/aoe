#include "../world/game/game_settings.hpp"

namespace aoe {

ScenarioSettings::ScenarioSettings()
	: players(), owners()
	, fixed_start(true), explored(false), all_technologies(false), cheating(false)
	, square(true), wrap(false), restricted(true), reorder(false), width(Terrain::min_size), height(Terrain::min_size)
	, popcap(100)
	, age(1), seed(1), villagers(3), type(TerrainType::normal)
	, res(200, 200, 0, 0)
{
	players.emplace_back("Gaia", 0, 0, Resources());
}

void ScenarioSettings::remove(IdPoolRef ref) {
	owners.erase(ref);
}

}
