#pragma once

#include <idpool.hpp>

#include "entity_info.hpp"

namespace aoe {

enum class EntityState {
	alive,
	dying,
	decaying,
	attack,
	attack_follow,
	moving,
};

enum class EntityTaskType {
	move,
	infer, // use context to determine task
	attack,
	train_unit,
};

class Entity;

class EntityView final {
public:
	IdPoolRef ref;
	EntityType type; // TODO remove this and use stats.type

	unsigned playerid;
	float x, y, angle;

	// TODO add ui info
	unsigned subimage;
	EntityState state;
	bool xflip;

	EntityStats stats;

	EntityView();
	EntityView(const Entity&);
};
class EntityTask final {
public:
	EntityTaskType type;
	IdPoolRef ref1, ref2;
	uint32_t x, y;
	unsigned info_type, info_value;

	// TODO use floating point number x,y
	// move to area
	EntityTask(IdPoolRef ref, uint32_t x, uint32_t y);
	EntityTask(EntityTaskType type, IdPoolRef ref1, IdPoolRef ref2);
	// train unit
	EntityTask(IdPoolRef ref, EntityType t);
};


}
