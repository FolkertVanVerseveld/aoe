#include "../server.hpp"

namespace aoe {

ClientInfo &Server::get_ci(IdPoolRef ref) {
	Peer p(refs.at(ref).sock, "", "", false);
	return peers.at(p);
}

bool Server::process_playermod(const Peer &p, NetPlayerControl &ctl, std::deque<uint8_t> &out) {
	NetPkg pkg;

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

}
