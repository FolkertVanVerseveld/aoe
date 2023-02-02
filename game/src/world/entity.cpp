#include "../game.hpp"

namespace aoe {

void Entity::imgtick(unsigned n) {
	// TODO use angle to determine where it's looking at
	// TOOD use xflip for other half of images
	switch (type) {
	case EntityType::villager:
		subimage = (subimage + n) % 6;
		break;
	case EntityType::bird1:
		subimage = (subimage + n) % 12;
		break;
	}
}

}
