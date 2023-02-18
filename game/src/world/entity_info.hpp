#pragma once

#include <vector>
#include <string>

namespace aoe {

enum class EntityType {
	town_center,
	barracks,
	bird1,
	villager,
	priest,
	berries,
	desert_tree1,
	desert_tree2,
	desert_tree3,
	desert_tree4,
};

enum class EntityIconType {
	building,
	unit,
	resource,
};

struct EntityInfo final {
	EntityType type;
	float size;
	unsigned hp;
	unsigned icon;
	std::string name;
};

struct EntityStats final {
	EntityType type;
	unsigned hp, maxhp;

	EntityStats(const EntityInfo &i) : type(i.type), hp(i.hp), maxhp(i.hp) {}
};

extern const std::vector<EntityInfo> entity_info;

}
