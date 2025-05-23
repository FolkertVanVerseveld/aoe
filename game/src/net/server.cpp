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
	Peer p(refs.at(ref).sock, "", "", false);
	return peers.at(p);
}

bool Server::process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out) {
	pkg.ntoh();

	// TODO for broadcasts, check packet on bogus data if reusing pkg
	switch (pkg.type()) {
		case NetPkgType::set_protocol:
			return chk_protocol(p, out, pkg);
		case NetPkgType::chat_text:
			broadcast(pkg);
			break;
		case NetPkgType::start_game:
			start_game(p);
			break;
		case NetPkgType::set_scn_vars: {
			auto scn = pkg.get_scn_vars();
			return set_scn_vars(p, scn);
		}
		case NetPkgType::set_username:
			return chk_username(p, out, pkg.username());
		case NetPkgType::client_info:
			return process_clientinfo(p, pkg);
		case NetPkgType::playermod: {
			auto cntl = pkg.get_player_control();
			return process_playermod(p, cntl, out);
		}
		case NetPkgType::entity_mod: {
			auto ent = pkg.get_entity_mod();
			return process_entity_mod(p, ent, out);
		}
		case NetPkgType::cam_set: {
			auto cam = pkg.get_cam_set();
			return cam_set(p, cam);
		}
		case NetPkgType::gamespeed_control: {
			auto spd = pkg.get_gamespeed();
			gamespeed_control(p, spd);
			break;
		}
		default:
			fprintf(stderr, "bad type: %u\n", (unsigned)pkg.type());
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

void Server::start_game(const Peer &p) {
	if (m_running || !p.is_host)
		return;

	// only start if all clients are ready
	lock lk(m_peers);

	bool ready = true;

	for (auto kv : peers) {
		ClientInfo &ci = kv.second;
		if (!(ci.flags & (unsigned)ClientInfoFlags::ready)) {
			ready = false;
			break;
		}
	}

	if (!ready) {
		NetPkg pkg;
		pkg.set_chat_text(invalid_ref, "Click \"I\'m ready!\" so the game can start");
		broadcast(pkg);
		return;
	}

	std::thread t([this]() {
		m_running = true;
		w.eventloop(*this, nullptr);
	});
	t.detach();
}

}
