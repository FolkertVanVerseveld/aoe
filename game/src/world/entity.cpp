#include "../game.hpp"

#include <array>

#include <cmath>

namespace aoe {

EntityView::EntityView(const Entity &e) : ref(e.ref), type(e.type), color(e.color), x(e.x), y(e.y), angle(e.angle), subimage(e.subimage), state(e.state), xflip(e.xflip) {}

Entity::Entity(IdPoolRef ref) : ref(ref), type(EntityType::town_center), color(0), x(0), y(0), angle(0), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(EntityState::alive), xflip(false) {}

Entity::Entity(IdPoolRef ref, EntityType type, unsigned color, float x, float y, float angle, EntityState state) : ref(ref), type(type), color(color), x(x), y(y), angle(angle), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(state), xflip(false) {}

Entity::Entity(const EntityView &ev) : ref(ev.ref), type(ev.type), color(ev.color), x(ev.x), y(ev.y), angle(ev.angle), target_ref(invalid_ref), target_x(0), target_y(0), subimage(ev.subimage), state(ev.state), xflip(ev.xflip) {}

bool Entity::die() noexcept {
	if (state == EntityState::dying)
		return false;

	state = EntityState::dying;
	subimage = 0;

	return true;
}

void Entity::decay() noexcept {
	state = EntityState::decaying;
	subimage = 0;
}

template<typename T> constexpr int signum(T v) {
	return v > 0 ? 1 : v < 0 ? -1 : 0;
}

bool Entity::tick() noexcept {
	if (state == EntityState::moving) {
		if (target_ref != invalid_ref)
			return false;

		float dx = target_x - x, dy = target_y - y;
		float distance = sqrt(dx * dx + dy * dy);

		float speed = 0.04f;
		if (distance < speed) {
			x = target_x;
			y = target_y;
			state = EntityState::alive;
			return true;
		}
		angle = fmodf(atan2(dy, dx) + 2 * M_PI, 2 * M_PI);

		x += speed * cos(angle);
		y += speed * sin(angle);

		return true;
	}

	return false;
}

bool Entity::task_move(float x, float y) noexcept {
	if (!is_alive() || is_building(type))
		return false;

	// TODO start movement stuff
	this->target_x = x;
	this->target_y = y;
	this->state = EntityState::moving;

	return true;
}

std::optional<SfxId> Entity::sfxtick() noexcept {
	switch (type) {
	case EntityType::villager:
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
	// TODO use angle to determine where it's looking at
	// TOOD use xflip for other half of images
	bool more = true;
	unsigned mult;

	xflip = false;

	const std::array<unsigned, 8> faces{ 1, 2, 3, 4, 3, 2, 1, 0 };
	const std::array<bool, 8> xflips{ true, true, true, false, false, false, false, false };

	float normface = fmodf(angle + M_PI / 8, 2 * M_PI) / (M_PI / 4);
	unsigned uface = (unsigned)normface % 8u;

	unsigned face = faces[uface];
	xflip = xflips[uface];

	switch (type) {
	case EntityType::villager:
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
		case EntityState::moving: {
			mult = 15;
			subimage = (subimage + n) % mult + face * mult;
			break;
		}
		default:
			mult = 6;
			subimage = (subimage + n) % mult + face * mult;
			break;
		}
		break;
	case EntityType::bird1:
		subimage = (subimage + n) % 12;
		break;
	case EntityType::priest:
		switch (state) {
		case EntityState::decaying:
			more = subimage < 5;
			subimage = std::min(subimage + n, 5u);
			break;
		case EntityState::dying:
			more = subimage < 9;
			subimage = std::min(subimage + n, 9u);
			break;
		case EntityState::attack:
			more = subimage != 0; // image of impact
			subimage = (subimage + n) % 10;
			break;
		case EntityState::alive:
		default:
			subimage = (subimage + n) % 10;
			break;
		}
		break;
	}

	return more;
}

}
