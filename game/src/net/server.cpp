#include "../server.hpp"

#include "../legacy/legacy.hpp"

namespace aoe {

const Peer *Server::try_peer(IdPoolRef ref) {
	std::lock_guard<std::mutex> lk(m_peers);
	Peer p(refs.at(ref).sock, "", "", false);

	auto it = peers.find(p);
	if (it == peers.end())
		return nullptr;

	return &it->first;
}

ClientInfo &Server::get_ci(IdPoolRef ref) {
	std::lock_guard<std::mutex> lk(m_peers);
	Peer p(refs.at(ref).sock, "", "", false);
	return peers.at(p);
}

bool Server::process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out) {
	pkg.ntoh();

	// TODO for broadcasts, check packet on bogus data if reusing pkg
	switch (pkg.type()) {
		case NetPkgType::set_protocol:
			return chk_protocol(p, out, pkg.protocol_version());
		case NetPkgType::chat_text:
			broadcast(pkg);
			break;
		case NetPkgType::start_game:
			start_game();
			break;
		case NetPkgType::set_scn_vars:
			return set_scn_vars(p, pkg.get_scn_vars());
		case NetPkgType::set_username:
			return chk_username(p, out, pkg.username());
		case NetPkgType::playermod:
			return process_playermod(p, pkg.get_player_control(), out);
		case NetPkgType::entity_mod:
			return process_entity_mod(p, pkg.get_entity_mod(), out);
		case NetPkgType::cam_set:
			return cam_set(p, pkg.get_cam_set());
		case NetPkgType::gamespeed_control:
			gamespeed_control(p, pkg.get_gamespeed());
			break;
		default:
			fprintf(stderr, "bad type: %u\n", pkg.type());
			throw "invalid type";
	}

	return true;
}

IdPoolRef Server::peer2ref(const Peer &p) {
	return peers.at(p).ref;
}

bool Server::cam_set(const Peer &p, NetCamSet &cam) {
	IdPoolRef ref = peer2ref(p);
	w.add_event(ref, WorldEventType::peer_cam_move, EventCameraMove(ref, cam));

	return true;
}

void Server::gamespeed_control(const Peer &p, const NetGamespeedControl &control) {
	IdPoolRef ref = peer2ref(p);
	w.add_event(ref, WorldEventType::gamespeed_control, control);
}

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

void Server::start_game() {
	if (m_running)
		return;

	m_running = true;

	std::thread t([this]() {
		w.eventloop(*this);
	});
	t.detach();
}

}
