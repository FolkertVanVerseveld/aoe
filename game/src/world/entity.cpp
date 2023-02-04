#include "../game.hpp"

#include <cmath>

namespace aoe {

EntityView::EntityView(const Entity &e) : ref(e.ref), type(e.type), color(e.color), x(e.x), y(e.y), angle(e.angle), subimage(e.subimage), state(e.state) {}

Entity::Entity(IdPoolRef ref) : ref(ref), type(EntityType::town_center), color(0), x(0), y(0), angle(0), target_ref(invalid_ref), target_x(0), target_y(0), subimage(0), state(EntityState::alive) {}

Entity::Entity(const EntityView &ev) : ref(ev.ref), type(ev.type), color(ev.color), x(ev.x), y(ev.y), angle(0), target_ref(invalid_ref), target_x(0), target_y(0), subimage(ev.subimage), state(ev.state) {}

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
		float angle = atan2(dy, dx);

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

	switch (type) {
	case EntityType::villager:
		switch (state) {
		case EntityState::dying:
			more = subimage < 9;
			subimage = std::min(subimage + n, 9u);
			break;
		case EntityState::decaying:
			more = subimage < 5;
			subimage = std::min(subimage + n, 5u);
			break;
		case EntityState::attack:
			more = subimage != 9; // image of impact
			subimage = (subimage + n) % 15;
			break;
		case EntityState::moving:
			subimage = (subimage + n) % 15;
			break;
		default:
			subimage = (subimage + n) % 6;
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
