#include "../../server.hpp"

#include "../../engine.hpp"

#include <cstdio>

namespace aoe {

void Client::entitymod(const NetEntityMod &em) {
	std::lock_guard<std::mutex> lk(m);

	switch (em.type) {
	case NetEntityControlType::add:
		g.entity_add(std::get<EntityView>(em.data));
		break;
	case NetEntityControlType::kill:
		g.entity_kill(std::get<IdPoolRef>(em.data));
		break;
	case NetEntityControlType::update:
		g.entity_update(std::get<EntityView>(em.data));
		break;
	case NetEntityControlType::spawn:
		g.entity_spawn(std::get<EntityView>(em.data));
		break;
	default:
		fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)em.type);
		break;
	}
}

}
