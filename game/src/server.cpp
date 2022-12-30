#include "server.hpp"

#include "engine.hpp"
#include "engine/audio.hpp"

namespace aoe {

Server::Server() : ServerSocketController(), s(), m_active(false), m_running(false), m_peers(), port(0), protocol(0), peers(), refs(), scn(), logic_gamespeed(1.0), t() {}

Server::~Server() {
	stop();
}

struct pkg {
	uint16_t length;
	uint16_t type;
};

bool Server::incoming(ServerSocket &s, const Peer &p) {
	std::lock_guard<std::mutex> lk(m_peers);

	if (peers.size() > 255)
		return false;

	std::string name(p.host + ":" + p.server);
	auto ins = refs.emplace(p.sock);
	IdPoolRef ref(ins.first->first);
	peers[p] = ClientInfo(ref, name);

	NetPkg pkg;

	pkg.set_incoming(ref);
	broadcast(pkg);

	pkg.set_chat_text(invalid_ref, name + " joined");
	broadcast(pkg);

	pkg.set_username(name);
	send(p, pkg);

	pkg.set_player_resize(scn.players.size());
	send(p, pkg);

	// now send all other player refs we have to the joined peer
	for (auto kv : peers) {
		IdPoolRef r = kv.second.ref;

		if (ref == r)
			continue;

		// TODO check if username has been set

		pkg.set_ref_username(r, kv.second.username);
		send(p, pkg);
	}

	return true;
}

void Server::dropped(ServerSocket &s, const Peer &p) {
	std::lock_guard<std::mutex> lk(m_peers);

	ClientInfo ci(peers.at(p));
	std::string name(ci.username);

	refs.erase(ci.ref);
	peers.erase(p);

	NetPkg pkg;

	pkg.set_dropped(ci.ref);
	broadcast(pkg);

	pkg.set_chat_text(invalid_ref, name + " left");
	broadcast(pkg);
}

void Server::stopped() {
	std::lock_guard<std::mutex> lk(m_peers);
	peers.clear();
}

int Server::proper_packet(ServerSocket &s, const std::deque<uint8_t> &q) {
	if (q.size() < NetPkgHdr::size)
		return 0;

	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	static_assert(sizeof(data) == NetPkgHdr::size);

	for (unsigned i = 0; i < NetPkgHdr::size; ++i)
		data.b[i] = q[i];

	NetPkgHdr h(data.v[0], data.v[1], false);
	h.ntoh();

	return q.size() - NetPkgHdr::size >= h.payload;
}

void Server::broadcast(NetPkg &pkg, bool include_host) {
	std::vector<uint8_t> v;
	pkg.write(v);
	s.broadcast(v.data(), (int)v.size(), include_host);
}

/**
 * Send packet to all peers except \a exclude.
 */
void Server::broadcast(NetPkg &pkg, const Peer &exclude)
{
	std::vector<uint8_t> v;
	pkg.write(v);

	for (auto kv : peers) {
		const Peer &p = kv.first;

		if (p.sock == exclude.sock)
			continue;


		s.send(p, v.data(), v.size());
	}
}

void Server::send(const Peer &p, NetPkg &pkg) {
	std::vector<uint8_t> v;
	pkg.write(v);
	s.send(p, v.data(), v.size());
}

bool Server::chk_protocol(const Peer &p, std::deque<uint8_t> &out, uint16_t req) {
	printf("%s: (%s,%s) requests protocol %u. answer protocol %u\n", __func__, p.host.c_str(), p.server.c_str(), req, protocol);

	NetPkg pkg;
	pkg.set_protocol(protocol);
	pkg.write(out);

	return true;
}

void Server::change_username(const Peer &p, std::deque<uint8_t> &out, const std::string &name) {
	auto it = peers.find(p);
	assert(it != peers.end());

	NetPkg pkg;
	ClientInfo &ci = it->second;
	pkg.set_username(ci.username = name);
	pkg.write(out);

	pkg.set_ref_username(ci.ref, name);
	broadcast(pkg, p);
}

bool Server::chk_username(const Peer &p, std::deque<uint8_t> &out, const std::string &name) {
	std::lock_guard<std::mutex> lk(m_peers);

	auto it = peers.find(p);
	if (it == peers.end())
		return false;

	std::string old(it->second.username);

	printf("%s: (%s,%s) wants to change username from \"%s\" to \"%s\"\n", __func__, p.host.c_str(), p.server.c_str(), old.c_str(), name.c_str());

	// allow anything when it's unique and doesn't contain `:'. we use : ourselves when generating names
	size_t pos = name.find(':');
	if (pos != std::string::npos) {
		// : found. ignore and send back old name
		NetPkg pkg;
		pkg.set_username(old);
		pkg.write(out);

		return true;
	}

	// check if name is unique
	for (auto kv : peers) {
		const std::string &n = kv.second.username;

		// ignore our old name
		if (n == old)
			continue;

		if (n == name) {
			// duplicate found. add `:' and port name, which is guaranteed to be unique
			change_username(p, out, name + ":" + p.server);
			return true;
		}
	}

	// success, send username back to client
	change_username(p, out, name);
	return true;
}

bool Server::set_scn_vars(const Peer &p, ScenarioSettings &scn) {
	if (!p.is_host) {
		// if restricted, client is modded: kick now!
		fprintf(stderr, "hacked client %s:%s: kick!\n", p.host.c_str(), p.server.c_str());
		return false;
	}

	// TODO filter bogus settings
	this->scn.width = scn.width;
	this->scn.height = scn.height;
	this->scn.popcap = scn.popcap;
	this->scn.age = scn.age;
	this->scn.seed = scn.seed;
	this->scn.villagers = scn.villagers;

	this->scn.res = scn.res;

	this->scn.fixed_start = scn.fixed_start;
	this->scn.explored = scn.explored;
	this->scn.all_technologies = scn.all_technologies;
	this->scn.cheating = scn.cheating;
	this->scn.square = scn.square;
	this->scn.wrap = scn.wrap;

	t.resize(this->scn.width, this->scn.height, this->scn.seed, this->scn.players.size(), this->scn.wrap);

	NetPkg pkg;
	pkg.set_scn_vars(this->scn);
	broadcast(pkg);

	return true;
}

bool Server::process_playermod(const Peer &p, NetPlayerControl &ctl, std::deque<uint8_t> &out) {
	NetPkg pkg;

	switch (ctl.type) {
		case NetPlayerControlType::resize:
			scn.players.resize(ctl.arg);
			pkg.set_player_resize(ctl.arg);
			broadcast(pkg);
			return true;
		case NetPlayerControlType::set_ref: {
			unsigned idx = ctl.arg; // remember, it is 1-based

			// ignore bad index, might be desync
			if (!idx || idx > scn.players.size())
				return true;

			// claim slot. NOTE multiple players can claim the same slot
			IdPoolRef ref = peers.at(p).ref;
			scn.owners[ref] = idx;

			// sent to players
			pkg.set_claim_player(ref, idx);
			broadcast(pkg);

			return true;
		}
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

	pkg.set_scn_vars(scn);
	broadcast(pkg);

	this->t.resize(scn.width, scn.height, scn.seed, scn.players.size(), scn.wrap);
	this->t.generate();

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
		default:
			fprintf(stderr, "bad type: %u\n", pkg.type());
			throw "invalid type";
	}

	return true;
}

bool Server::process_packet(ServerSocket &s, const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed) {
	NetPkg pkg(in);
	return process(p, pkg, out);
}

int Server::mainloop(int, uint16_t port, uint16_t protocol) {
	this->port = port;
	this->protocol = protocol;

	m_active = true;
	int r = s.mainloop(port, 10, *this);

	return r;
}

void Server::stop() {
	m_running = m_active = false;
}

void Server::close() {
	stop();
	s.close();
}

Client::Client() : s(), port(0), m_connected(false), m(), peers(), me(invalid_ref), scn(), g() {}

Client::~Client() {
	stop();
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

void Client::start_game() {
	std::lock_guard<std::mutex> lk(m_eng);
	puts("start game");
	if (eng)
		eng->trigger_multiplayer_started();
}

void Client::set_scn_vars(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m_eng);

	g.resize(scn);

	if (eng)
		eng->set_scn_vars(scn);
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

void Client::playermod(const NetPlayerControl &ctl) {
	std::lock_guard<std::mutex> lk(m_eng);
	if (eng)
		eng->trigger_playermod(ctl);
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
		case NetPeerControlType::set_player_idx:
			scn.owners[ref] = std::get<uint16_t>(ctl.data);
			break;
		default:
			fprintf(stderr, "%s: unknown type: %u\n", __func__, (unsigned)ctl.type);
			break;
	}
}

void Client::terrainmod(const NetTerrainMod &tm) {
	g.terrain_set(tm.tiles, tm.hmap, tm.x, tm.y, tm.w, tm.h);
}

void Client::mainloop() {
	send_protocol(1);

	try {
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
				case NetPkgType::start_game:
					start_game();
					break;
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
	pkg.claim_player_setting(me, idx);
	send(pkg);
}

void Client::send_scn_vars(const ScenarioSettings &scn) {
	//printf("%s: %u,%u\n", __func__, scn.width, scn.height);
	NetPkg pkg;
	pkg.set_scn_vars(scn);
	send(pkg);
}

void Client::send_players_resize(unsigned n) {
	NetPkg pkg;
	pkg.set_player_resize(n);
	send(pkg);
}

}
