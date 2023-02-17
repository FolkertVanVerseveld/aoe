#include "entity_info.hpp"

#include "../legacy.hpp"

namespace aoe {

using namespace io;

const std::vector<EntityInfo> entity_info{
	{EntityType::town_center, 3, 600, (unsigned)BldIcon::towncenter1, "Town Center"},
	{EntityType::barracks, 3, 350, (unsigned)BldIcon::barracks1, "Barracks"},
	{EntityType::bird1, 1, 1, (unsigned)UnitIcon::villager, "Bird"},
	{EntityType::villager, 1, 40, (unsigned)UnitIcon::villager, "Villager"},
	{EntityType::priest, 1, 25, (unsigned)UnitIcon::priest, "Priest"},
	// hp for berries refers to amount of food
	{EntityType::berries, 1, 150, (unsigned)UnitIcon::food, "Berry Bush"},
	{EntityType::desert_tree1, 1, 25, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree2, 1, 25, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree3, 1, 25, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree4, 1, 25, (unsigned)UnitIcon::wood, "Tree - Palm"},
};

}
