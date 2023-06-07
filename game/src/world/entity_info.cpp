#include "entity_info.hpp"

namespace aoe {

using namespace io;

const std::vector<EntityBldInfo> entity_bld_info {
	{EntityType::town_center, DrsId::bld_town_center, DrsId::bld_town_center_player, DrsId::bld_debris},
	{EntityType::barracks, DrsId::bld_barracks, DrsId::bld_barracks, DrsId::bld_debris},
};

const std::vector<EntityImgInfo> entity_img_info {
	// alive, dying, decaying, attack, attack_follow, moving
	// alive, dying, ii_dying, decaying, ii_decaying, attack, ii_attack, attack_follow, moving
	// dying, decaying, attack, attack_follow, moving, alive
	{EntityType::bird1, 12, 12, 0, 12, 12, 0, 12, 12, DrsId::gif_bird1, DrsId::gif_bird1, DrsId::gif_bird1, DrsId::gif_bird1, DrsId::gif_bird1, DrsId::gif_bird1},
	{EntityType::villager, 6, 10, 10-1, 6, 15, 15-1, 15, 15, DrsId::gif_villager_die1, DrsId::gif_villager_decay, DrsId::gif_villager_attack, DrsId::gif_villager_move, DrsId::gif_villager_move, DrsId::gif_villager_stand},
	{EntityType::worker_wood1, 6, 10, 10-1, 6, 11, 11-1, 15, 15, DrsId::gif_worker_wood_die, DrsId::gif_worker_wood_decay, DrsId::gif_worker_wood_attack1, DrsId::gif_worker_wood_move, DrsId::gif_worker_wood_move, DrsId::gif_worker_wood_stand},
	{EntityType::worker_wood2, 6, 10, 10-1, 6, 11, 11-1, 15, 15, DrsId::gif_worker_wood_die, DrsId::gif_worker_wood_decay, DrsId::gif_worker_wood_attack2, DrsId::gif_worker_wood_move, DrsId::gif_worker_wood_move, DrsId::gif_worker_wood_stand},
	{EntityType::worker_gold, 6, 10, 10-1, 6, 11, 11-1, 15, 15, DrsId::gif_worker_miner_die, DrsId::gif_worker_miner_decay, DrsId::gif_worker_miner_attack, DrsId::gif_worker_miner_move, DrsId::gif_worker_miner_move, DrsId::gif_worker_miner_stand},
	{EntityType::worker_stone, 6, 10, 10-1, 6, 11, 11-1, 15, 15, DrsId::gif_worker_miner_die, DrsId::gif_worker_miner_decay, DrsId::gif_worker_miner_attack, DrsId::gif_worker_miner_move, DrsId::gif_worker_miner_move, DrsId::gif_worker_miner_stand},
	{EntityType::worker_berries, 6, 10, 10-1, 6, 27, 19, 15, 15, DrsId::gif_villager_die1, DrsId::gif_villager_decay, DrsId::gif_worker_berries_attack, DrsId::gif_villager_move, DrsId::gif_villager_move, DrsId::gif_villager_stand},
	{EntityType::melee1, 6, 10, 10-1, 6, 15, 15-1, 15, 15, DrsId::gif_melee1_die, DrsId::gif_melee1_decay, DrsId::gif_melee1_attack, DrsId::gif_melee1_move, DrsId::gif_melee1_move, DrsId::gif_melee1_stand},
	{EntityType::priest, 10, 10, 10-1, 6, 10, 10-1, 15, 15, DrsId::gif_priest_die, DrsId::gif_priest_decay, DrsId::gif_priest_attack, DrsId::gif_priest_move, DrsId::gif_priest_move, DrsId::gif_priest_stand},
};

const std::vector<EntityInfo> entity_info {
	{EntityType::town_center, 3, 600, 0, (unsigned)BldIcon::towncenter1, "Town Center"},
	{EntityType::barracks, 3, 350, 0, (unsigned)BldIcon::barracks1, "Barracks"},
	{EntityType::bird1, 5, 1, 0, (unsigned)UnitIcon::villager, "Bird"},
	{EntityType::villager, 5, 25, 3, (unsigned)UnitIcon::villager, "Villager"},
	{EntityType::worker_wood1, 5, 25, 3, (unsigned)UnitIcon::villager, "Lumberjack"}, // attacking tree
	{EntityType::worker_wood2, 5, 25, 1, (unsigned)UnitIcon::villager, "Lumberjack"}, // gather wood
	{EntityType::worker_gold, 5, 25, 1, (unsigned)UnitIcon::villager, "Gold Miner"}, // gather gold
	{EntityType::worker_stone, 5, 25, 1, (unsigned)UnitIcon::villager, "Stone Miner"}, // gather stone
	{EntityType::worker_stone, 5, 25, 1, (unsigned)UnitIcon::villager, "Forager"}, // gather stone
	{EntityType::melee1, 5, 40, 4, (unsigned)UnitIcon::melee1, "Clubman"},
	// a priest converts units and does not deal damage while converting
	{EntityType::priest, 5, 25, 0, (unsigned)UnitIcon::priest, "Priest"},
	// hp for berries refers to amount of food
	{EntityType::berries, 8, 150, 0, (unsigned)UnitIcon::food, "Berry Bush"},
	{EntityType::gold, 12, 400, 0, (unsigned)UnitIcon::gold, "Gold"},
	{EntityType::stone, 12, 250, 0, (unsigned)UnitIcon::stone, "Stone"},
	{EntityType::desert_tree1, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree2, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree3, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	{EntityType::desert_tree4, 5, 25, 0, (unsigned)UnitIcon::wood, "Tree - Palm"},
	// hp for tree refers to amount of wood
	{EntityType::dead_tree1, 5, 40, 0, (unsigned)UnitIcon::wood, "Tree"},
	{EntityType::dead_tree2, 5, 40, 0, (unsigned)UnitIcon::wood, "Tree"},
};

}
