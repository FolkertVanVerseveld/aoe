#include "../../server.hpp"

#include <cstdio>

namespace aoe {

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
			uint16_t size = std::clamp<uint16_t>(std::get<uint16_t>(ctl.data), 1, MAX_PLAYERS);
			w.scn.players.resize(size);
			pkg.set_player_resize(size);
			broadcast(pkg);
			return true;
		}
		case NetPlayerControlType::set_ref: {
			unsigned idx = std::get<uint16_t>(ctl.data);

			// ignore bad index, might be desync. also allow 0 to release ref
			if (idx >= w.scn.players.size())
				return true;

			// claim slot. NOTE multiple players can claim the same slot
			IdPoolRef ref = peers.at(p).ref;
			w.scn.owners[ref] = idx;

			// send to players
			pkg.set_claim_player(ref, idx);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_civ: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);
			unsigned idx = p.first;

			// ignore bad index, might be desync
			if (idx >= w.scn.players.size())
				return true;

			unsigned civ = p.second;

			if (civ >= civnames.size())
				return false; // kick, cuz we have data inconsistency

			w.scn.players[idx].civ = civ;

			pkg.set_player_civ(idx, civ);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_team: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);
			unsigned idx = p.first;

			// ignore bad index, might be desync
			if (idx >= w.scn.players.size())
				return true;

			unsigned team = p.second;

			w.scn.players[idx].team = team;

			pkg.set_player_team(idx, team);
			broadcast(pkg);

			return true;
		}
		case NetPlayerControlType::set_player_name: {
			auto p = std::get<std::pair<uint16_t, std::string>>(ctl.data);
			printf("set %u to \"%s\"\n", p.first, p.second.c_str());

			unsigned idx = p.first;

			// ignore bad index, might be desync
			if (idx >= w.scn.players.size())
				return true;

			w.scn.players[idx].name = p.second;

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

}
