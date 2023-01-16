#include "../server.hpp"

namespace aoe {

void Client::send_players_resize(unsigned n) {
	NetPkg pkg;
	pkg.set_player_resize(n);
	send(pkg);
}

void Client::send_set_player_name(unsigned idx, const std::string &s) {
	NetPkg pkg;
	pkg.set_player_name(idx, s);
	send(pkg);
}

void Client::send_set_player_civ(unsigned idx, unsigned civ) {
	NetPkg pkg;
	pkg.set_player_civ(idx, civ);
	send(pkg);
}

void Client::send_set_player_team(unsigned idx, unsigned team) {
	NetPkg pkg;
	pkg.set_player_team(idx, team);
	send(pkg);
}

void Client::playermod(const NetPlayerControl &ctl) {
	std::lock_guard<std::mutex> lk(m);

	switch (ctl.type) {
		case NetPlayerControlType::resize:
			scn.players.resize(std::get<uint16_t>(ctl.data));
			break;
		case NetPlayerControlType::erase: {
			unsigned pos = std::get<uint16_t>(ctl.data);

			if (pos < scn.players.size())
				scn.players.erase(scn.players.begin() + pos);

			break;
		}
		case NetPlayerControlType::set_cpu_ref: {
			unsigned pos = std::get<uint16_t>(ctl.data) - 1; // remember, it is 1-based

			if (pos < scn.players.size())
				scn.players[pos].ai = true;

			// remove any refs
			for (auto it = scn.owners.begin(); it != scn.owners.end();) {
				if (it->second == pos)
					it = scn.owners.erase(it);
				else
					++it;
			}
			break;
		}
		case NetPlayerControlType::set_player_name: {
			auto p = std::get<std::pair<uint16_t, std::string>>(ctl.data);

			unsigned pos = p.first - 1;

			if (pos < scn.players.size())
				scn.players[pos].name = p.second;

			break;
		}
		case NetPlayerControlType::set_civ: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);

			unsigned pos = p.first - 1;

			if (pos < scn.players.size())
				scn.players[pos].civ = p.second;

			break;
		}
		case NetPlayerControlType::set_team: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);

			unsigned pos = p.first - 1;

			if (pos < scn.players.size())
				scn.players[pos].team = p.second;

			break;
		}
		default:
			fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)ctl.type);
			break;
	}

	modflags |= (unsigned)ClientModFlags::scn;
}

}
