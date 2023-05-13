#include "../game.hpp"

#include <array>

#include <cmath>

#include "../server.hpp"
#include "entity_info.hpp"

namespace aoe {

EntityView::EntityView() : ref(invalid_ref), type(EntityType::town_center), playerid(0), x(0), y(0), angle(0), subimage(0), state(EntityState::alive), xflip(false), stats(entity_info.at((unsigned)type)) {}

EntityView::EntityView(const Entity &e) : ref(e.ref), type(e.type), playerid(e.playerid), x(e.x), y(e.y), angle(e.angle), subimage(e.subimage), state(e.state), xflip(e.xflip), stats(e.stats) {}

Entity::Entity(IdPoolRef ref) : ref(ref), type(EntityType::town_center), playerid(0), x(0), y(0), angle(0), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(EntityState::alive), xflip(false), stats(entity_info.at((unsigned)type)) {}

Entity::Entity(IdPoolRef ref, EntityType type, unsigned playerid, float x, float y, float angle, EntityState state) : ref(ref), type(type), playerid(playerid), x(x), y(y), angle(angle), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(state), xflip(false), stats(entity_info.at((unsigned)type)) {}

Entity::Entity(const EntityView &ev) : ref(ev.ref), type(ev.type), playerid(ev.playerid), x(ev.x), y(ev.y), angle(ev.angle), target_ref(invalid_ref), target_x(0), target_y(0), subimage(ev.subimage), state(ev.state), xflip(ev.xflip), stats(ev.stats) {}

bool Entity::die() noexcept {
	if (state == EntityState::dying || state == EntityState::decaying)
		return false;

	stats.hp = 0;
	set_state(EntityState::dying);

	return true;
}

void Entity::decay() noexcept {
	set_state(EntityState::decaying);
}

void Entity::reset_anim() noexcept {
	subimage = 0;
	imgtick(0); // fix face depending on angle
}

bool Entity::set_state(EntityState s) noexcept {
	EntityState old = state;

	state = s;
	reset_anim();

	return old != s;
}

bool Entity::try_state(EntityState s) noexcept {
	if (state == s)
		return false;

	state = s;
	reset_anim();

	return true;
}

template<typename T> constexpr int signum(T v) {
	return v > 0 ? 1 : v < 0 ? -1 : 0;
}

float Entity::lookat(float x, float y) noexcept {
	float dx = x - this->x, dy = y - this->y;
	// compute angle and make sure it's always positive
	angle = fmodf(atan2(dy, dx) + 2 * M_PI, 2 * M_PI);
	return sqrt(dx * dx + dy * dy);
}

bool Entity::move() noexcept {
	float distance = lookat(target_x, target_y);

	float speed = 0.04f; // TODO determine from entity stats
	if (distance < speed) {
		x = target_x;
		y = target_y;
		return set_state(target_ref != invalid_ref ? EntityState::attack : EntityState::alive);
	}

	x += speed * cos(angle);
	y += speed * sin(angle);

	return true;
}

bool Entity::attack(WorldView &wv) noexcept {
	// TODO determine if we have to follow the target to stay in range of attack
	Entity *t = wv.try_get(target_ref);
	if (!t) {
		// cannot find target, just move there
		target_ref = invalid_ref;
		return set_state(EntityState::moving);
	}

	// stop attacking if dead or same player
	if (!t->is_alive() || t->playerid == playerid) {
		target_ref = invalid_ref;
		return set_state(EntityState::alive);
	}

	if (in_range(*t))
		return try_state(EntityState::attack);

	bool d = try_state(EntityState::attack_follow);
	target_x = t->x;
	target_y = t->y;
	return d |= move();
}

bool Entity::tick(WorldView &wv) noexcept {
	if (!stats.hp)
		return false;

	switch (state) {
	case EntityState::moving: return move();
	case EntityState::attack:
	case EntityState::attack_follow:
		return attack(wv);
	}

	return false;
}

bool Entity::hit(WorldView &wv, Entity &aggressor) noexcept {
	unsigned atk = aggressor.stats.attack;

	switch (aggressor.type) {
	case EntityType::priest:
		if (wv.try_convert(*this, aggressor)) {
			// TODO auto move priest to heal entity?
			aggressor.target_ref = invalid_ref;
			aggressor.set_state(EntityState::alive);
			return true;
		}

		break;
	default:
		if (stats.hp < atk) {
			if (is_resource(type)) {
				if (aggressor.type != EntityType::worker_wood1) {
					type = EntityType::dead_tree1;
					stats = entity_info.at((unsigned)type);
					return set_state(EntityState::decaying);
				}

				switch (type) {
				case EntityType::desert_tree1:
				case EntityType::desert_tree2:
				case EntityType::desert_tree3:
				case EntityType::desert_tree4:
					aggressor.type = EntityType::worker_wood2;

					type = EntityType::dead_tree1;
					stats = entity_info.at((unsigned)type);
					return set_state(EntityState::alive);
				}
			}

			return die();
		}

		stats.hp = std::max(stats.hp - atk, 0u);
		break;
	}

	// TODO do we need to recompute target_ref?

	// if we don't have a target ourselves, make this our target
	if (target_ref == invalid_ref)
		task_attack(aggressor);

	return true;
}

bool Entity::task_move(float x, float y) noexcept {
	if (!is_alive() || is_building(type) || is_resource(type))
		return false;

	// TODO start movement stuff
	this->target_ref = invalid_ref;
	this->target_x = x;
	this->target_y = y;
	this->state = EntityState::moving;

	return true;
}

bool Entity::task_attack(Entity &e) noexcept {
	// no we cannot attack ourself :p
	if (e.ref == ref)
		return false;

	// TODO add buildings that can attack
	if (!is_alive() || is_building(type) || is_resource(type) || !e.is_alive() || playerid == e.playerid)
		return false;

	this->target_ref = e.ref;
	// also set target_x, target_y in case the entity cannot be found anymore
	this->target_x = e.x;
	this->target_y = e.y;

	lookat(e.x, e.y);

	// villagers will change type when interacting with (non-)resources
	if (is_worker(type)) {
		type = EntityType::villager;

		// TODO update stats
		unsigned hp = stats.hp;
		stats = entity_info.at((unsigned)type);
		stats.hp = std::clamp(hp, 0u, stats.maxhp);

		if (is_resource(e.type)) {
			switch (e.type) {
			default:
				type = EntityType::worker_wood1;
				break;
			}
		}
	}

	set_state(in_range(e) ? EntityState::attack : EntityState::attack_follow);

	return true;
}

bool Entity::task_train_unit(EntityType type) noexcept {
	if (!is_alive() || !is_building(this->type) || is_resource(type))
		return false;

	switch (this->type) {
		case EntityType::town_center:
			return type == EntityType::villager;
		case EntityType::barracks:
			return type == EntityType::melee1;
	}

	return false;
}

std::optional<SfxId> Entity::sfxtick() noexcept {
	switch (type) {
	case EntityType::villager:
	case EntityType::melee1:
		switch (state) {
		case EntityState::attack:
			return SfxId::villager_attack_random;
		}
		break;
	case EntityType::worker_wood1:
	case EntityType::worker_wood2:
		if (state == EntityState::attack)
			return SfxId::wood_worker_attack;

		break;
	case EntityType::priest:
		switch (state) {
		case EntityState::attack:
			return SfxId::priest_attack_random;
		}
		break;
	}

	return std::nullopt;
}

bool Entity::in_range(const Entity &e) const noexcept {
	const EntityInfo &info1 = entity_info.at((unsigned)type);
	float size1 = is_building(type) ? info1.size / 2 : info1.size / 20;

	const EntityInfo &info2 = entity_info.at((unsigned)e.type);
	float size2 = is_building(e.type) ? info2.size / 2 : info2.size / 20;

	float x1 = x, x2 = e.x, y1 = y, y2 = e.y;

	if (is_building(type)) {
		x1 += 0.5f;
		y1 += 0.5f;
	}

	if (is_building(e.type)) {
		x2 += 0.5f;
		y2 += 0.5f;
	}

	float dx = x2 - x1, dy = y2 - y1;
	float distance = sqrt(dx * dx + dy * dy);

	// TODO get range, use 0.1f for now
	return distance < size1 + size2 + 0.05f;// +stats.range;
}

bool Entity::imgtick(unsigned n) noexcept {
	bool more = true;
	unsigned mult;

	const std::array<unsigned, 8> faces{ 1, 2, 3, 4, 3, 2, 1, 0 };

	// note that we have 8 faces, so (2pi)/8 = pi/4. then we need (2pi)/(2*8) to correct for [-a,+a) range.
	float normface = fmodf(angle + M_PI / 8, 2 * M_PI) / (M_PI / 4);
	unsigned uface = (unsigned)normface % 8u;

	unsigned face = faces[uface];
	xflip = uface < 3;

	if (type >= entity_img_info[0].type && type <= entity_img_info.back().type) {
		const EntityImgInfo &img = entity_img_info.at((unsigned)type - (unsigned)entity_img_info[0].type);

		switch (state) {
		case EntityState::alive:
			mult = img.alive;
			subimage = fmodf(subimage + n, mult) + face * mult;
			break;
		case EntityState::dying:
			mult = img.dying;
			more = (unsigned)fmodf(subimage, mult) != img.ii_dying;
			subimage = std::min<float>(fmodf(subimage, mult) + n, mult - 1) + face * mult;
			break;
		case EntityState::decaying:
			mult = img.decaying;
			more = (unsigned)fmodf(subimage, mult) != img.ii_decaying;
			subimage = std::min<float>(fmodf(subimage, mult) + n, mult - 1) + face * mult;
			break;
		case EntityState::attack:
			mult = img.attack;
			more = (unsigned)fmodf(subimage, mult) != img.ii_attack;
			subimage = fmodf(subimage + n, mult) + face * mult;
			break;
		case EntityState::attack_follow:
			mult = img.attack_follow;
			more = (unsigned)fmodf(subimage, mult) != img.attack_follow;
			subimage = fmodf(subimage + n, mult) + face * mult;
			break;
		case EntityState::moving:
			mult = img.moving;
			more = fmodf(subimage, mult) != img.moving;
			subimage = fmodf(subimage + n, mult) + face * mult;
			break;
		}
	}

#if 0
	switch (type) {
	case EntityType::bird1: {
		// TODO birds have more angles
		// TODO still buggy
		const std::array<unsigned, 16> faces{ 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0 };
		const std::array<bool, 16> xflips{ true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false };

		// note that we have 16 faces, so (2pi)/16 = pi/8. then we need (2pi)/(2*16) to correct for [-a,+a) range.
		float normface = fmodf(angle + M_PI / 16, 2 * M_PI) / (M_PI / 8);
		unsigned uface = (unsigned)normface % 16u;

		unsigned face = faces[uface];
		xflip = xflips[uface];

		mult = 12;
		subimage = (subimage + n) % mult + face * mult;
		break;
	}
	}
#endif

	return more;
}

}
