#include "entity_info.hpp"

#include "../legacy.hpp"

namespace aoe {

using namespace io;

const std::vector<EntityInfo> entity_info {
	{EntityType::town_center, 3, 600, 0, (unsigned)BldIcon::towncenter1, "Town Center"},
	{EntityType::barracks, 3, 350, 0, (unsigned)BldIcon::barracks1, "Barracks"},
	{EntityType::bird1, 5, 1, 0, (unsigned)UnitIcon::villager, "Bird"},
	{EntityType::villager, 5, 25, 3, (unsigned)UnitIcon::villager, "Villager"},
	{EntityType::melee1, 5, 40, 3, (unsigned)UnitIcon::melee1, "Clubman"},
	// a priest converts units and does not deal damage while converting
	{EntityType::priest, 5, 25, 0, (unsigned)UnitIcon::priest, "Priest"},
	// hp for berries refers to amount of food
	{EntityType::berries, 8, 150, 0, (unsigned)UnitIcon::food, "Berry Bush"},
	{EntityType::desert_tree1, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree2, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree3, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree4, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
};

}
