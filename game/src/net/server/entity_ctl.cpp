#include "../../server.hpp"

#include <cstdio>

namespace aoe {

bool Server::process_entity_mod(const Peer &p, NetEntityMod &em, std::deque<uint8_t> &out) {
	NetPkg pkg;

	if (!m_running)
		return false; // desync, kick

	IdPoolRef src = peer2ref(p);

	// reject entity control if peer is invalid
	if (src == invalid_ref && !p.is_host)
		return true;

	switch (em.type) {
	case NetEntityControlType::kill: {
		IdPoolRef ref = std::get<IdPoolRef>(em.data);
		w.add_event(src, WorldEventType::entity_kill, ref);

		return true;
	}
	case NetEntityControlType::task: {
		EntityTask task = std::get<EntityTask>(em.data);
		w.add_event(src, WorldEventType::entity_task, task);

		return true;
	}
	default:
		fprintf(stderr, "%s: bad entity control type %u\n", __func__, (unsigned)em.type);
		break;
	}

	return false;
}

}
