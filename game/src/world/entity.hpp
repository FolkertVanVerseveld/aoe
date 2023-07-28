#pragma once

#include <idpool.hpp>
#include <optional>

#include "../engine/audio.hpp"

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

class WorldView;

class Entity final {
public:
	IdPoolRef ref;
	EntityType type;

	unsigned playerid;
	float x, y, angle;

	IdPoolRef target_ref; // if == invalid_ref, use target_x,target_y
	float target_x, target_y;

	float subimage;
	EntityState state;
	bool xflip;

	EntityStats stats;

	Entity(IdPoolRef ref);
	Entity(IdPoolRef ref, EntityType type, unsigned playerid, float x, float y, float angle=0.0f, EntityState state=EntityState::alive);
	// resource only
	Entity(IdPoolRef ref, EntityType type, float x, float y, unsigned subimage);

	Entity(const EntityView &ev);

	friend bool operator<(const Entity &lhs, const Entity &rhs) noexcept {
		return lhs.ref < rhs.ref;
	}

	bool die() noexcept;
	void decay() noexcept;

	bool tick(WorldView&) noexcept;
	bool hit(WorldView&, Entity &aggressor) noexcept;

	unsigned get_atk(const EntityStats &stats) noexcept;

	constexpr bool is_alive() const noexcept {
		return state != EntityState::dying && state != EntityState::decaying;
	}

	bool task_cancel() noexcept;

	bool task_move(float x, float y) noexcept;
	bool task_attack(Entity&) noexcept;
	/** ensure specified type can be trained at this entity. */
	bool task_train_unit(EntityType) noexcept;

	std::optional<SfxId> sfxtick() noexcept;
	const EntityBldInfo &bld_info() const;
	const EntityImgInfo &img_info() const;

	bool imgtick(unsigned n) noexcept;
	void target_died(WorldView&);
private:
	void reset_anim() noexcept;
	bool set_state(EntityState) noexcept;
	bool try_state(EntityState) noexcept;
	bool move() noexcept;
	bool attack(WorldView&) noexcept;
	bool in_range(const Entity&) const noexcept;

	/* Compute facing angle and return euclidean distance. */
	float lookat(float x, float y) noexcept;

	void set_type(EntityType type, bool resethp=false);
};


}
