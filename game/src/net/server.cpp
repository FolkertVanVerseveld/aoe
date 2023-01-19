#include "../server.hpp"

namespace aoe {

ClientInfo &Server::get_ci(IdPoolRef ref) {
	Peer p(refs.at(ref).sock, "", "", false);
	return peers.at(p);
}

bool Server::process_entity_mod(const Peer &p, NetEntityMod &em, std::deque<uint8_t> &out) {
	NetPkg pkg;

	if (!m_running)
		return false; // desync, kick

	switch (em.type) {
	case NetEntityControlType::kill: {
		// TODO send command to game eventloop thread
		IdPoolRef ref = std::get<IdPoolRef>(em.data);

		// TODO check if player is allowed to kill this entity
		if (entities.try_invalidate(ref)) {
			pkg.set_entity_kill(ref);
			broadcast(pkg);
		}
		return true;
	}
	default:
		fprintf(stderr, "%s: bad entity control type %u\n", __func__, em.type);
		break;
	}

	return false;
}

bool Server::process_playermod(const Peer &p, NetPlayerControl &ctl, std::deque<uint8_t> &out) {
	NetPkg pkg;

	if (m_running) {
		// filter types that cannot be changed while the game is running
		switch (ctl.type) {
		case NetPlayerControlType::resize:
		case NetPlayerControlType::erase:
		case NetPlayerControlType::set_player_name:
		case NetPlayerControlType::set_civ:
		case NetPlayerControlType::set_team:
			return true;
		default:
			break;
		}
	}

	switch (ctl.type) {
		case NetPlayerControlType::resize: {
			uint16_t size = std::get<uint16_t>(ctl.data);
			w.scn.players.resize(size);
			pkg.set_player_resize(size);
			broadcast(pkg);
			return true;
		}
		case NetPlayerControlType::set_ref: {
			unsigned idx = std::get<uint16_t>(ctl.data); // remember, it is 1-based

			// ignore bad index, might be desync. also allow 0 to release ref
			if (idx > w.scn.players.size())
				return true;

			// claim slot. NOTE multiple players can claim the same slot
			IdPoolRef ref = peers.at(p).ref;
			w.scn.owners[ref] = idx;

			if (idx)
				w.scn.players[idx - 1].ai = false;

			// send to players
			pkg.set_claim_player(ref, idx);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_cpu_ref: {
			unsigned idx = std::get<uint16_t>(ctl.data); // remember, it is 1-based

			// ignore bad index, might be desync
			if (!idx || idx > w.scn.players.size())
				return true;

			// claim slot
			w.scn.players[idx - 1].ai = true;

			// purge any players that had this one claimed
			for (auto it = w.scn.owners.begin(); it != w.scn.owners.end();) {
				if (it->second == idx)
					it = w.scn.owners.erase(it);
				else
					++it;
			}

			// tell everyone and don't care to check which player had claimed this before
			pkg.set_cpu_player(idx);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_civ: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);
			unsigned idx = p.first; // remember, it is 1-based

			// ignore bad index, might be desync
			if (!idx || idx > w.scn.players.size())
				return true;

			unsigned civ = p.second;

			if (civ > civnames.size())
				return false; // kick, cuz we have data inconsistency

			w.scn.players[idx - 1].civ = civ;

			pkg.set_player_civ(idx, civ);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_team: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);
			unsigned idx = p.first; // remember, it is 1-based

			// ignore bad index, might be desync
			if (!idx || idx > w.scn.players.size())
				return true;

			unsigned team = p.second;

			w.scn.players[idx - 1].team = team;

			pkg.set_player_team(idx, team);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_player_name: {
			auto p = std::get<std::pair<uint16_t, std::string>>(ctl.data);
			printf("set %u to \"%s\"\n", p.first, p.second.c_str());

			unsigned idx = p.first;

			// ignore bad index, might be desync
			if (!idx || idx > w.scn.players.size())
				return true;

			w.scn.players[idx - 1].name = p.second;

			pkg.set_player_name(idx, p.second);
			broadcast(pkg);

			return true;
		}
		default:
			fprintf(stderr, "%s: bad playermod type: %u\n", __func__, ctl.type);
			break;
	}

	return false;
}

void Server::start_game() {
	if (m_running)
		return;

	m_running = true;

	std::thread t([this]() {
		w.eventloop(*this);
	});
	t.detach();

	NetPkg pkg;

	pkg.set_start_game();
	broadcast(pkg);

	pkg.set_scn_vars(w.scn);
	broadcast(pkg);

	for (unsigned i = 0; i < w.scn.players.size(); ++i) {
		PlayerSetting &p = w.scn.players[i];

		p.res = w.scn.res;

		// if player has no name, try find an owner that has one
		if (p.name.empty()) {
			unsigned owners = 0;
			std::string alias;

			for (auto kv : w.scn.owners) {
				if (kv.second == i + 1) {
					++owners;
					alias = get_ci(kv.first).username;
				}
			}

			if (owners == 1) {
				p.name = alias;
			} else {
				p.name = "Oerkneus de Eerste";

				if (p.civ >= 0 && p.civ < civs.size()) {
					auto &names = civs[civnames[p.civ]];
					p.name = names[rand() % names.size()];
				}
			}
		}

		pkg.set_player_name(i + 1, p.name);
		broadcast(pkg);
		pkg.set_player_civ(i + 1, p.civ);
		broadcast(pkg);
		pkg.set_player_team(i + 1, p.team);
		broadcast(pkg);
	}

	size_t size = w.scn.width * w.scn.height;
	w.create_terrain();

	players.clear();
	for (const PlayerSetting &ps : w.scn.players)
		players.emplace_back(ps, size);

	entities.clear();
	for (unsigned i = 0; i < players.size(); ++i) {
		// TODO wrap in spawn(towncenter) function or smth
		auto p = entities.emplace(EntityType::town_center, i, 2, 1 + 3 * i);
		assert(p.second);
		players[i].entities.emplace(p.first->first);

		p = entities.emplace(EntityType::barracks, i, 2 + 2 * 3, 1 + 4 * i);
		assert(p.second);
		players[i].entities.emplace(p.first->first);
	}

	unsigned w = 16, h = 16;
	NetTerrainMod tm(this->w.fetch_terrain(0, 0, w, h));

	pkg.set_terrain_mod(tm);
	broadcast(pkg);

	// now send all entities to each client
	for (auto &kv : entities) {
		pkg.set_entity_add(kv.second);
		// TODO only send to clients that can see this entity
		broadcast(pkg);
	}

	pkg.set_start_game();
	broadcast(pkg);
}

}
