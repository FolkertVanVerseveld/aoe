#include "../server.hpp"

namespace aoe {

ClientInfo &Server::get_ci(IdPoolRef ref) {
	Peer p(refs.at(ref).sock, "", "", false);
	return peers.at(p);
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
			scn.players.resize(size);
			pkg.set_player_resize(size);
			broadcast(pkg);
			return true;
		}
		case NetPlayerControlType::set_ref: {
			unsigned idx = std::get<uint16_t>(ctl.data); // remember, it is 1-based

			// ignore bad index, might be desync. also allow 0 to release ref
			if (idx > scn.players.size())
				return true;

			// claim slot. NOTE multiple players can claim the same slot
			IdPoolRef ref = peers.at(p).ref;
			scn.owners[ref] = idx;

			if (idx)
				scn.players[idx - 1].ai = false;

			// sent to players
			pkg.set_claim_player(ref, idx);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_cpu_ref: {
			unsigned idx = std::get<uint16_t>(ctl.data); // remember, it is 1-based

			// ignore bad index, might be desync
			if (!idx || idx > scn.players.size())
				return true;

			// claim slot
			scn.players[idx - 1].ai = true;

			// purge any players that had this one claimed
			for (auto it = scn.owners.begin(); it != scn.owners.end();) {
				if (it->second == idx)
					it = scn.owners.erase(it);
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
			if (!idx || idx > scn.players.size())
				return true;

			unsigned civ = p.second;

			if (civ > civnames.size())
				return false; // kick, cuz we have data inconsistency

			scn.players[idx - 1].civ = civ;

			pkg.set_player_civ(idx, civ);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_team: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);
			unsigned idx = p.first; // remember, it is 1-based

			// ignore bad index, might be desync
			if (!idx || idx > scn.players.size())
				return true;

			unsigned team = p.second;

			scn.players[idx - 1].team = team;

			pkg.set_player_team(idx, team);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_player_name: {
			auto p = std::get<std::pair<uint16_t, std::string>>(ctl.data);
			printf("set %u to \"%s\"\n", p.first, p.second.c_str());

			unsigned idx = p.first;

			// ignore bad index, might be desync
			if (!idx || idx > scn.players.size())
				return true;

			scn.players[idx - 1].name = p.second;

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
		eventloop();
	});
	t.detach();

	NetPkg pkg;

	pkg.set_start_game();
	broadcast(pkg);

	pkg.set_scn_vars(scn);
	broadcast(pkg);

	for (unsigned i = 0; i < scn.players.size(); ++i) {
		PlayerSetting &p = scn.players[i];

		p.res = scn.res;

		// if player has no name, try find an owner that has one
		if (p.name.empty()) {
			unsigned owners = 0;
			std::string alias;

			for (auto kv : scn.owners) {
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

	size_t size = scn.width * scn.height;
	this->t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	this->t.generate();

	entities.clear();

	players.clear();
	for (const PlayerSetting &ps : scn.players)
		players.emplace_back(ps, size);

	NetTerrainMod tm;

	unsigned w = std::min(16u, this->t.w), h = std::min(16u, this->t.h);
	this->t.fetch(tm.tiles, tm.hmap, 0, 0, w, h);

	tm.x = tm.y = 0;
	tm.w = w; tm.h = h;

	pkg.set_terrain_mod(tm);
	broadcast(pkg);

	pkg.set_start_game();
	broadcast(pkg);
}

}
