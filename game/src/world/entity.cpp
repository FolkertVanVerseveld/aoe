#include "../game.hpp"

namespace aoe {

EntityView::EntityView(const Entity &e) : ref(e.ref), type(e.type), color(e.color), x(e.x), y(e.y), angle(e.angle), subimage(e.subimage), state(e.state) {}

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

bool Entity::imgtick(unsigned n) noexcept {
	// TODO use angle to determine where it's looking at
	// TOOD use xflip for other half of images
	bool more = true;

	switch (type) {
	case EntityType::villager:
		switch (state) {
		case EntityState::dying:
			more = subimage < 9u;
			subimage = std::min(subimage + n, 9u);
			break;
		default:
			subimage = (subimage + n) % 6;
			break;
		}
		break;
	case EntityType::bird1:
		subimage = (subimage + n) % 12;
		break;
	}

	return more;
}

}
