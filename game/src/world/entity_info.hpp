#pragma once

#include <vector>
#include <string>

namespace aoe {

enum class EntityType {
	town_center,
	barracks,
	bird1,
	villager,
	worker_wood1,
	worker_wood2,
	melee1,
	priest,
	berries,
	desert_tree1,
	desert_tree2,
	desert_tree3,
	desert_tree4,
	dead_tree1,
	dead_tree2,
};

enum class EntityIconType {
	building,
	unit,
	resource,
};

struct EntityImgInfo final {
	EntityType type;
	// ii = Image of Impact
	unsigned alive, dying, ii_dying, decaying;
	unsigned attack, ii_attack, attack_follow, moving;
};

struct EntityInfo final {
	EntityType type;
	float size;
	unsigned hp;
	unsigned atk;
	unsigned icon;
	std::string name;

	// TODO add upgrade function that copies everything except hp
};

struct EntityStats final {
	EntityType type;
	unsigned hp, maxhp;
	unsigned attack;

	EntityStats(const EntityInfo &i) : type(i.type), hp(i.hp), maxhp(i.hp), attack(i.atk) {}
};

extern const std::vector<EntityInfo> entity_info;
extern const std::vector<EntityImgInfo> entity_img_info;

}
