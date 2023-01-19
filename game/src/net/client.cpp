#include "../server.hpp"

#include "../engine.hpp"

namespace aoe {

Client::Client() : s(), port(0), m_connected(false), starting(false), m(), peers(), me(invalid_ref), scn(), g(), modflags(-1) {}

Client::~Client() {
	stop();
}

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

void Client::entitymod(const NetEntityMod &em) {
	std::lock_guard<std::mutex> lk(m);

	switch (em.type) {
	case NetEntityControlType::add:
		g.entity_add(std::get<EntityView>(em.data));
		break;
	case NetEntityControlType::kill:
		g.entity_kill(std::get<IdPoolRef>(em.data));
		break;
	default:
		fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)em.type);
		break;
	}
}

void Client::start_game() {
	std::lock_guard<std::mutex> lk(m_eng);

	g.set_players(scn.players);

	puts("start game");
	if (eng)
		eng->trigger_multiplayer_started();
}

void Client::entity_kill(IdPoolRef ref) {
	NetPkg pkg;
	pkg.set_entity_kill(ref);
	send(pkg);
}

void Client::send_protocol(uint16_t version) {
	NetPkg pkg;
	pkg.set_protocol(version);
	send(pkg);
}

uint16_t Client::recv_protocol() {
	NetPkg pkg = recv();
	return pkg.protocol_version();
}

void Client::send_chat_text(const std::string &s) {
	NetPkg pkg;
	pkg.set_chat_text(me, s);
	send(pkg);
}

void Client::send_start_game() {
	NetPkg pkg;
	pkg.set_start_game();
	send(pkg);
}

void Client::send_username(const std::string &s) {
	NetPkg pkg;
	pkg.set_username(s);
	send(pkg);
}

void Client::claim_player(unsigned idx) {
	NetPkg pkg;
	pkg.claim_player_setting(idx);
	send(pkg);
}

void Client::claim_cpu(unsigned idx) {
	NetPkg pkg;
	pkg.claim_cpu_setting(idx);
	send(pkg);
}

void Client::send_scn_vars(const ScenarioSettings &scn) {
	NetPkg pkg;
	pkg.set_scn_vars(scn);
	send(pkg);
}

}
