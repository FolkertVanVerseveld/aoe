#pragma once

#include <vector>
#include <string>

#include "../legacy/legacy.hpp"
#include "resources.hpp"

namespace aoe {

enum class EntityType {
	town_center,
	barracks,
	bird1,
	villager,
	worker_wood1,
	worker_wood2,
	worker_gold,
	worker_stone,
	worker_berries,
	melee1,
	priest,
	berries,
	gold,
	stone,
	desert_tree1,
	desert_tree2,
	desert_tree3,
	desert_tree4,
	grass_tree1,
	grass_tree2,
	grass_tree3,
	grass_tree4,
	dead_tree1,
	dead_tree2,
};

static inline EntityType desert_tree(unsigned pos) {
	return (EntityType)((unsigned)EntityType::desert_tree1 + pos % 4);
}

static inline EntityType grass_tree(unsigned pos) {
	return (EntityType)((unsigned)EntityType::grass_tree1 + pos % 4);
}

static bool constexpr is_building(EntityType t) {
	return t >= EntityType::town_center && t <= EntityType::barracks;
}

static bool constexpr is_resource(EntityType t) {
	return t >= EntityType::berries && t <= EntityType::dead_tree2;
}

static bool constexpr is_worker(EntityType t) {
	return t >= EntityType::villager && t <= EntityType::worker_berries;
}

static bool constexpr is_tree(EntityType t) {
	return t >= EntityType::desert_tree1 && t <= EntityType::dead_tree2;
}

// TODO add is_unit (when needed?)

enum class EntityIconType {
	building,
	unit,
	resource,
};

struct EntityBldInfo final {
	EntityType type;
	io::DrsId slp_base, slp_player, slp_die;
};

struct EntityImgInfo final {
	EntityType type;
	// ii = Image of Impact
	unsigned alive, dying, ii_dying, decaying;
	unsigned attack, ii_attack, attack_follow, moving;
	io::DrsId slp_die, slp_decay, slp_attack, slp_attack_follow, slp_moving, slp_alive;
};

struct EntityInfo final {
	EntityType type;
	float size;
	unsigned hp;
	unsigned atk, atk_bld;
	unsigned icon; // unsigned as it can have different types
	std::string name;
	Resources cost;

	// TODO add upgrade function that copies everything except hp
};

struct EntityStats final {
	EntityType type;
	unsigned hp, maxhp;
	unsigned attack, attack_bld;

	EntityStats(const EntityInfo &i) : type(i.type), hp(i.hp), maxhp(i.hp), attack(i.atk), attack_bld(i.atk_bld) {}
};

extern const std::vector<EntityInfo> entity_info;
extern const std::vector<EntityImgInfo> entity_img_info;
extern const std::vector<EntityBldInfo> entity_bld_info;

}
