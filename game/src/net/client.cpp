#include "../server.hpp"

#include "../engine.hpp"

namespace aoe {

Client::Client() : s(), port(0), m_connected(false), starting(false), m(), peers(), me(invalid_ref), scn(), g(), modflags(-1), gameover(false) {}

Client::~Client() {
	stop();
}

void Client::mainloop() {
	send_protocol(1);

	try {
		starting = false;

		while (m_connected) {
			NetPkg pkg = recv();

			switch (pkg.type()) {
				case NetPkgType::set_protocol:
					printf("prot=%u\n", pkg.protocol_version());
					break;
				case NetPkgType::chat_text: {
					auto p = pkg.chat_text();
					add_chat_text(p.first, p.second);
					break;
				}
				case NetPkgType::start_game: {
					if (starting) {
						start_game();
					} else {
						starting = true;
						add_chat_text(invalid_ref, "game starting now");
					}
					break;
				}
				case NetPkgType::gameover: {
					gameover = true;
					EngineView ev;
					ev.play_sfx(SfxId::gameover_defeat);
					break;
				}
				case NetPkgType::set_scn_vars:
					set_scn_vars(pkg.get_scn_vars());
					break;
				case NetPkgType::set_username:
					set_username(pkg.username());
					break;
				case NetPkgType::playermod:
					playermod(pkg.get_player_control());
					break;
				case NetPkgType::peermod:
					peermod(pkg.get_peer_control());
					break;
				case NetPkgType::terrainmod:
					terrainmod(pkg.get_terrain_mod());
					break;
				case NetPkgType::entity_mod:
					entitymod(pkg.get_entity_mod());
					break;
				case NetPkgType::gameticks:
					gameticks(pkg.get_gameticks());
					break;
				default:
					printf("%s: type=%X\n", __func__, pkg.type());
					break;
			}
		}
	} catch (std::runtime_error &e) {
		if (m_connected)
			fprintf(stderr, "%s: client stopped: %s\n", __func__, e.what());
	}

	std::lock_guard<std::mutex> lk(m_eng);
	if (eng) {
		eng->trigger_multiplayer_stop();
		if (m_connected && !eng->is_hosting()) {
			eng->push_error("Game session aborted");
		}
	}
}

void Client::gameticks(unsigned n) {
	ZoneScoped;
	g.tick(n);
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
		case NetPlayerControlType::died: {
			unsigned pos = std::get<uint16_t>(ctl.data);
			g.player_died(pos);
			break;
		}
		case NetPlayerControlType::set_cpu_ref: {
			unsigned pos = std::get<uint16_t>(ctl.data);

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

			unsigned pos = p.first;

			if (pos < scn.players.size())
				scn.players[pos].name = p.second;

			break;
		}
		case NetPlayerControlType::set_civ: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);

			unsigned pos = p.first;

			if (pos < scn.players.size())
				scn.players[pos].civ = p.second;

			break;
		}
		case NetPlayerControlType::set_team: {
			auto p = std::get<std::pair<uint16_t, uint16_t>>(ctl.data);

			unsigned pos = p.first;

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
	case NetEntityControlType::update:
		g.entity_update(std::get<EntityView>(em.data));
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

void Client::cam_move(float x, float y, float w, float h) {
	NetPkg pkg;
	pkg.cam_set(x, y, w, h);
	send(pkg);
}

void Client::send_gamespeed_control(const NetGamespeedControl &control) {
	NetPkg pkg;
	pkg.set_gamespeed(control.type);
	send(pkg);
}

void Client::entity_move(IdPoolRef ref, float x, float y) {
	NetPkg pkg;
	pkg.entity_move(ref, x, y);
	send(pkg);
}

void Client::entity_infer(IdPoolRef r1, IdPoolRef r2) {
	NetPkg pkg;
	pkg.entity_task(r1, r2);
	send(pkg);
}

void Client::send_scn_vars(const ScenarioSettings &scn) {
	NetPkg pkg;
	pkg.set_scn_vars(scn);
	send(pkg);
}

void Client::stop() {
	std::lock_guard<std::mutex> lk(m);
	m_connected = false;
	s.close();
}

void Client::start(const char *host, uint16_t port, bool run) {
	std::lock_guard<std::mutex> lk(m);
	s.open();
	m_connected = false;

	s.connect(host, port);
	m_connected = true;

	if (run) {
		std::thread t(&Client::mainloop, std::ref(*this));
		t.detach();
	}
}

void Client::add_chat_text(IdPoolRef ref, const std::string &s) {
	std::lock_guard<std::mutex> lk(m_eng);

	const ClientInfo *ci = nullptr;
	std::string txt(s);

	if (ref != invalid_ref) {
		txt = std::string("(") + std::to_string(ref.first) + "::" + std::to_string(ref.second) + "): " + s;

		auto it = peers.find(ref);
		if (it != peers.end())
			ci = &it->second;

		if (ci && !ci->username.empty())
			txt = ci->username + ": " + s;
	}

	printf("(%u::%u) says: \"%s\"\n", ref.first, ref.second, s.c_str());
	if (eng) {
		eng->add_chat_text(txt);

		auto maybe_taunt = eng->sfx.is_taunt(s);
		if (maybe_taunt)
			eng->sfx.play_taunt(maybe_taunt.value());
	}
}

void Client::set_scn_vars(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m);

	this->scn.fixed_start = scn.fixed_start;
	this->scn.explored = scn.explored;
	this->scn.all_technologies = scn.all_technologies;
	this->scn.cheating = scn.cheating;
	this->scn.square = scn.square;
	this->scn.wrap = scn.wrap;

	// scn.restricted is not copied for obvious reasons

	this->scn.width = scn.width;
	this->scn.height = scn.height;
	this->scn.popcap = scn.popcap;
	this->scn.age = scn.age;
	this->scn.villagers = scn.villagers;

	this->scn.res = scn.res;
	this->modflags |= (unsigned)ClientModFlags::scn;

	g.resize(scn);
}

void Client::set_username(const std::string &s) {
	std::lock_guard<std::mutex> lk(m_eng);

	if (me == invalid_ref) {
		fprintf(stderr, "%s: got username before ref\n", __func__);
	} else {
		auto it = peers.find(me);
		assert(it != peers.end());
		it->second.username = s;
	}

	if (eng)
		eng->trigger_username(s);
}

void Client::peermod(const NetPeerControl &ctl) {
	IdPoolRef ref(ctl.ref);

	switch (ctl.type) {
		case NetPeerControlType::incoming: {
			// name = (%u::%u)
			std::string name(std::string("(") + std::to_string(ref.first) + "::" + std::to_string(ref.second) + ")");
			peers.emplace(std::piecewise_construct, std::forward_as_tuple(ref), std::forward_as_tuple(ref, name));

			if (me == invalid_ref)
				me = ref;
			break;
		}
		case NetPeerControlType::dropped: {
			peers.erase(ref);

			if (me == ref)
				fprintf(stderr, "%s: dropping myself. ref (%u,%u)\n", __func__, ref.first, ref.second);
			break;
		}
		case NetPeerControlType::set_username: {
			auto it = peers.find(ref);
			std::string name(std::get<std::string>(ctl.data));

			// insert if not found
			if (it == peers.end())
				auto ins = peers.emplace(std::piecewise_construct, std::forward_as_tuple(ref), std::forward_as_tuple(ref, name));
			else
				it->second.username = name;

			break;
		}
		case NetPeerControlType::set_player_idx: {
			std::lock_guard<std::mutex> lk(m);

			unsigned pos = std::get<uint16_t>(ctl.data);
			scn.owners[ref] = pos;

			if (pos - 1 < scn.players.size())
				scn.players[pos - 1].ai = false;

			modflags |= (unsigned)ClientModFlags::scn;
			break;
		}
		default:
			fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)ctl.type);
			break;
	}
}

void Client::terrainmod(const NetTerrainMod &tm) {
	std::lock_guard<std::mutex> lk(m);
	modflags |= (unsigned)ClientModFlags::terrain;
	g.terrain_set(tm.tiles, tm.hmap, tm.x, tm.y, tm.w, tm.h);
}

}
