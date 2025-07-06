#include "../server.hpp"

#include "../engine.hpp"
#include "../debug.hpp"

namespace aoe {

IClient::IClient()
	: m(), me(invalid_ref), scn(), modflags(-1), playerindex(0), team_me(0)
	, victory(false), gameover(false), m_connected(false), g() {}

LocalClient::LocalClient()
	: IClient(), IServer(), w() {}

void LocalClient::close() {
	set_active(false);
	stop();
}

void LocalClient::set_scn_vars(const ScenarioSettings &scn) {
	lock lk(m);

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

void LocalClient::broadcast(NetPkg &pkg, bool include_host) {
	switch (pkg.type()) {
	case NetPkgType::start_game:
		if (pkg.get_start_type() == NetStartGameType::now)
		{
			g.set_players(sp_scn.players); // FIXME players not set properly
			EngineView ev;
			ev.goto_menu(MenuState::singleplayer_game);
		}
		break;
	case NetPkgType::set_scn_vars:
		set_scn_vars(pkg.get_scn_vars());
		break;
	default:
		break;
	}
}

void LocalClient::stop() {
	lock lk(m);
	m_connected = false;
}

void LocalClient::event_loop(const ScenarioSettings &scn, UI_TaskInfo &info) {
	ZoneScoped;
	m_connected = true;

	w.load_sp_players();
	w.load_scn(scn);
	w.eventloop(*this, &info);
}

void LocalClient::claim_player(unsigned idx) {
	// TODO do we need this?
}

void LocalClient::cam_move(float x, float y, float w, float h) {
	// TODO use this
}

void LocalClient::send_gamespeed_control(NetGamespeedType type) {
	// TODO use this
}

void LocalClient::send_chat_text(const std::string &s) {
	// TODO stub
	puts(s.c_str());
}

void LocalClient::send_start_game() {
	// TODO do we need this?
}

void LocalClient::send_ready(bool) {
	// TODO do we need this?
}

void LocalClient::send_players_resize(unsigned n) {
	// TODO use this
	//sp_scn.players.resize(n);
}

void LocalClient::send_set_player_name(unsigned idx, const std::string &s) {
	// TODO use this
}

void LocalClient::send_set_player_civ(unsigned idx, unsigned civ) {
	// TODO use this
}

void LocalClient::send_set_player_team(unsigned idx, unsigned team) {
	// TODO use this
}

void LocalClient::send_scn_vars(const ScenarioSettings &scn) {
	// TODO use this
}

void LocalClient::send_username(const std::string &s) {
	// TODO do we need this?
}

void LocalClient::entity_move(IdPoolRef ref, float x, float y) {
	// TODO use this
}

void LocalClient::entity_infer(IdPoolRef r1, IdPoolRef r2) {
	// TODO use this
}

void LocalClient::entity_train(IdPoolRef ref, EntityType type) {
	// TODO use this
}

void LocalClient::entity_kill(IdPoolRef ref) {
	// TODO use this
}

Client::Client(const std::string &username)
	: IClient()
	, s(), port(0), starting(false), peers()
	, sendbuf(), initial_username(username), gamespeed(0) {}

void Client::mainloop() {
	send_protocol(1);

	try {
		starting = false;

		while (m_connected) {
			NetPkg pkg = recv();

			switch (pkg.type()) {
				case NetPkgType::set_protocol:
					LOGF("prot=%u\n", pkg.protocol_version());
					send_username(initial_username);
					break;
				case NetPkgType::chat_text: {
					auto p = pkg.chat_text();
					add_chat_text(p.first, p.second);
					break;
				}
				case NetPkgType::start_game: {
					switch (pkg.get_start_type()) {
					case NetStartGameType::now:
						start_game();
						break;
					case NetStartGameType::announce:
						starting = true;
						add_chat_text(invalid_ref, "game starting now");
						break;
					}
					break;
				}
				case NetPkgType::gameover: {
					std::lock_guard<std::mutex> lk(m);
					g.gameover(pkg.get_gameover());

					// TODO this is racey, victory may be determined later
					auto maybe_pv = g.try_pv(playerindex);
					if (maybe_pv.has_value()) {
						unsigned me_team = maybe_pv.value().init.team;
						victory = me_team == g.winning_team();
					} else {
						LOGF("%s: unable to determine winning team: playerindex=%u\n", __func__, playerindex);
						victory = false;
					}
					gameover = true;

					EngineView ev;

					if (victory)
						ev.play_sfx(SfxId::gameover_victory);
					else
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
				case NetPkgType::gamespeed_control:
					gamespeed_control(pkg.get_gamespeed());
					break;
				case NetPkgType::particle_mod:
					particlemod(pkg);
					break;
				case NetPkgType::resmod:
					resource_ctl(pkg);
					break;
				default:
					LOGF("%s: unknown type %u\n", __func__, (unsigned)pkg.type());
					break;
			}
		}
	} catch (std::runtime_error &e) {
		if (m_connected)
			LOGF(stderr, "%s: client stopped: %s\n", __func__, e.what());
	}

	std::lock_guard<std::mutex> lk(m_eng);
	if (eng) {
		eng->trigger_multiplayer_stop();
		if (m_connected && !eng->is_hosting())
			eng->push_error("Game session aborted");
	}
}

void Client::gameticks(unsigned n) {
	ZoneScoped;
	g.tick(n);
}

void Client::gamespeed_control(const NetGamespeedControl &ctl) {
	ZoneScoped;
	NetGamespeedType type = ctl.type;

	uint8_t old_speed = this->gamespeed, new_speed = ctl.speed;
	this->gamespeed = new_speed;

	bool new_running = new_speed != 0;
	g.running = new_running;

	if (old_speed == new_speed)
		return;

	lock lk(m_eng);
	if (!eng)
		return;

	if (new_speed == 0) {
		eng->add_chat_text("Game paused");
	} else if (old_speed == 0) {
		eng->add_chat_text("Game resumed"); 
	} else {
		char buf[80];
		float speed;

		speed = ctl.speed * World::gamespeed_step;
		snprintf(buf, sizeof buf, "Gamespeed set to %.1f", speed);

		eng->add_chat_text(buf);
	}
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

void Client::entity_train(IdPoolRef src, EntityType type) {
	NetPkg pkg;
	pkg.entity_train(src, type);
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
		case NetPlayerControlType::set_score: {
			auto p = std::get<NetPlayerScore>(ctl.data);
			g.set_player_score(p.playerid, p);
			break;
		}
		default:
			fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)ctl.type);
			break;
	}

	modflags |= (unsigned)ClientModFlags::scn;
}

void Client::start_game() {
	std::lock_guard<std::mutex> lk(m_eng);

	g.set_players(scn.players);

	puts("start game");
	if (eng)
		eng->trigger_async_flags(EngineAsyncTask::multiplayer_started);
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

void Client::send_ready(bool v) {
	NetPkg pkg;
	pkg.set_ready(v);
	send(pkg);
}

void Client::send_start_game() {
	NetPkg pkg;
	pkg.set_start_game(NetStartGameType::now);
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

void Client::cam_move(float x, float y, float w, float h) {
	NetPkg pkg;
	pkg.cam_set(x, y, w, h);
	send(pkg);
}

void Client::send_gamespeed_control(NetGamespeedType type) {
	NetPkg pkg;
	uint8_t speed = 0;

	switch (type) {
	case NetGamespeedType::increase:
		speed = this->gamespeed + 1;
		break;
	case NetGamespeedType::decrease:
		speed = this->gamespeed - 1;
		break;
	}

	pkg.set_gamespeed(type, speed);
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
	lock lk(m);
	m_connected = false;
	s.close();
}

void Client::start(const char *host, uint16_t port, bool run) {
	lock lk(m);
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
	lock lk(m_eng);

	const ClientInfo *ci = nullptr;
	std::string txt(s);

	if (ref != invalid_ref) {
		txt = std::string("(") + std::to_string(ref.first) + "::" + std::to_string(ref.second) + "): " + s;

		lock lk(m);
		auto it = peers.find(ref);
		if (it != peers.end())
			ci = &it->second;

		if (ci && !ci->username.empty())
			txt = ci->username + ": " + s;
	}

	LOGF("(%u::%u) says: \"%s\"\n", ref.first, ref.second, s.c_str());
	if (eng) {
		eng->add_chat_text(txt);

		auto maybe_taunt = eng->sfx.is_taunt(s);
		if (maybe_taunt)
			eng->sfx.play_taunt(maybe_taunt.value());
	}
}

void Client::set_scn_vars(const ScenarioSettings &scn) {
	lock lk(m);

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
	lock lk(m_eng);

	if (me == invalid_ref) {
		fprintf(stderr, "%s: got username before ref\n", __func__);
	} else {
		lock lk(m);
		auto it = peers.find(me);
		assert(it != peers.end());
		it->second.username = s;
	}

	if (eng)
		eng->trigger_username(s);
}

/** set me to \a ref. NOTE assuming mutex \a m has already been acquired. */
void Client::set_me(IdPoolRef ref) {
	me = ref;
	modflags |= (unsigned)ClientModFlags::ref;
}

void Client::peermod(const NetPeerControl &ctl) {
	IdPoolRef ref(ctl.ref);
	std::lock_guard<std::mutex> lk(m);

	switch (ctl.type) {
		case NetPeerControlType::incoming: {
			// name = (%u::%u)
			std::string name(std::string("(") + std::to_string(ref.first) + "::" + std::to_string(ref.second) + ")");
			peers.emplace(std::piecewise_construct, std::forward_as_tuple(ref), std::forward_as_tuple(ref, name));

			break;
		}
		case NetPeerControlType::dropped: {
			peers.erase(ref);

			if (me == ref)
				LOGF(stderr, "%s: dropping myself. ref (%u,%u)\n", __func__, ref.first, ref.second);
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
			unsigned pos = std::get<uint16_t>(ctl.data);
			scn.owners[ref] = pos;

			if (ref == me)
				playerindex = pos;

			modflags |= (unsigned)ClientModFlags::scn;
			break;
		}
		case NetPeerControlType::set_peer_ref:
			set_me(ref);
			break;
		default:
			fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)ctl.type);
			break;
	}
}

void Client::terrainmod(const NetTerrainMod &tm) {
	lock lk(m);
	modflags |= (unsigned)ClientModFlags::terrain;
	g.terrain_set(tm.tiles, tm.hmap, tm.x, tm.y, tm.w, tm.h);
}

}
