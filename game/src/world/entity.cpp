#include "../game.hpp"

#include <array>

#include <cmath>

#include "../server.hpp"

namespace aoe {

EntityView::EntityView() : ref(invalid_ref), type(EntityType::town_center), color(0), x(0), y(0), angle(0), subimage(0), state(EntityState::alive), xflip(false), stats(entity_info.at((unsigned)type)) {}

EntityView::EntityView(const Entity &e) : ref(e.ref), type(e.type), color(e.color), x(e.x), y(e.y), angle(e.angle), subimage(e.subimage), state(e.state), xflip(e.xflip), stats(e.stats) {}

Entity::Entity(IdPoolRef ref) : ref(ref), type(EntityType::town_center), color(0), x(0), y(0), angle(0), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(EntityState::alive), xflip(false), stats(entity_info.at((unsigned)type)) {}

Entity::Entity(IdPoolRef ref, EntityType type, unsigned color, float x, float y, float angle, EntityState state) : ref(ref), type(type), color(color), x(x), y(y), angle(angle), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(state), xflip(false), stats(entity_info.at((unsigned)type)) {}

Entity::Entity(const EntityView &ev) : ref(ev.ref), type(ev.type), color(ev.color), x(ev.x), y(ev.y), angle(ev.angle), target_ref(invalid_ref), target_x(0), target_y(0), subimage(ev.subimage), state(ev.state), xflip(ev.xflip), stats(ev.stats) {}

bool Entity::die() noexcept {
	if (state == EntityState::dying)
		return false;

	stats.hp = 0;
	set_state(EntityState::dying);

	return true;
}

void Entity::decay() noexcept {
	set_state(EntityState::decaying);
}

bool Entity::set_state(EntityState s) noexcept {
	EntityState old = state;
	state = s;
	// reset animation
	subimage = 0;
	imgtick(0); // fix face depending on angle
	return old != s;
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
		state = target_ref != invalid_ref ? EntityState::attack : EntityState::alive;
		return true;
	}

	x += speed * cos(angle);
	y += speed * sin(angle);

	return true;
}

bool Entity::attack(WorldView &wv) noexcept {
	// TODO determine if we have to follow the target to stay in range of attack
	Entity *t = wv.try_get(target_ref);
	if (!t)
		// cannot find target, just move there
		return set_state(EntityState::attack_follow);

	if (!t->is_alive()) {
		target_ref = invalid_ref;
		return set_state(EntityState::alive);
	}

	// TODO stub
	return false;
}

bool Entity::tick(WorldView &wv) noexcept {
	if (!stats.hp)
		return false;

	switch (state) {
	case EntityState::attack_follow:
	case EntityState::moving: return move();
	case EntityState::attack: return attack(wv);
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
		if (stats.hp < atk)
			return die();

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
	if (!is_alive() || is_building(type))
		return false;

	// TODO start movement stuff
	this->target_ref = invalid_ref;
	this->target_x = x;
	this->target_y = y;
	this->state = EntityState::moving;

	return true;
}

bool Entity::task_attack(Entity &e) noexcept {
	// TODO add buildings that can attack
	if (!is_alive() || is_building(type) || is_resource(type) || !e.is_alive())
		return false;

	// TODO introduce attack_follow state where we make sure we are in range of attack

	this->target_ref = e.ref;
	// also set target_x, target_y in case the entity cannot be found anymore
	this->target_x = e.x;
	this->target_y = e.y;

	lookat(e.x, e.y);
	set_state(EntityState::attack);

	return true;
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
	case EntityType::priest:
		switch (state) {
		case EntityState::attack:
			return SfxId::priest_attack_random;
		}
		break;
	}

	return std::nullopt;
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

	switch (type) {
	case EntityType::villager:
	case EntityType::melee1:
		switch (state) {
		case EntityState::dying:
			mult = 10;
			more = subimage % mult < mult - 1;
			subimage = std::min(subimage % mult + n, mult - 1) + face * mult;
			break;
		case EntityState::decaying:
			mult = 6;
			more = subimage % mult < mult - 1;
			subimage = std::min(subimage % mult + n, mult - 1) + face * mult;
			break;
		case EntityState::attack:
			mult = 15;
			more = subimage % mult != 9; // image of impact
			subimage = (subimage + n) % mult + face * mult;
			break;
		case EntityState::moving:
			mult = 15;
			subimage = (subimage + n) % mult + face * mult;
			break;
		default:
			mult = 6;
			subimage = (subimage + n) % mult + face * mult;
			break;
		}
		break;
	case EntityType::bird1: {
#if 0
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
#else
		face = 0;
		mult = 12;
		subimage = (subimage + n) % mult + face * mult;
#endif
		break;
	}
	case EntityType::priest:
		switch (state) {
		case EntityState::decaying:
			mult = 6;
			more = subimage % mult < mult - 1;
			subimage = std::min(subimage % mult + n, mult - 1) + face * mult;
			break;
		case EntityState::dying:
			mult = 10;
			more = subimage % mult < mult - 1;
			subimage = std::min(subimage % mult + n, mult - 1) + face * mult;
			break;
		case EntityState::attack:
			mult = 10;
			more = subimage % mult != 0; // image of impact
			subimage = (subimage + n) % mult + face * mult;
			break;
		case EntityState::alive:
			mult = 10;
			subimage = (subimage + n) % mult + face * mult;
			break;
		case EntityState::moving:
			mult = 15;
			subimage = (subimage + n) % mult + face * mult;
			break;
		}
		break;
	}

	return more;
}

}
