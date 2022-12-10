#include "server.hpp"

#include "engine.hpp"

namespace aoe {

Server::Server() : ServerSocketController(), s(), m_active(false), m_peers(), port(0), protocol(0), peers(), scn() {}

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
	peers[p] = ClientInfo(name);

	NetPkg pkg;

	pkg.set_chat_text(name + " joined");
	broadcast(pkg);

	pkg.set_username(name);
	send(p, pkg);

	pkg.set_player_resize(scn.players.size());
	send(p, pkg);

	return true;
}

void Server::dropped(ServerSocket &s, const Peer &p) {
	std::lock_guard<std::mutex> lk(m_peers);
	std::string name(peers.at(p).username);
	peers.erase(p);

	NetPkg pkg;
	pkg.set_chat_text(name + " left");

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
	pkg.set_username(it->second.username = name);
	pkg.write(out);

	// TODO broadcast new name
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
			// duplicate found. add : and port name
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
	}

	return false;
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
			broadcast(pkg);
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
	m_active = false;
}

void Server::close() {
	stop();
	s.close();
}

Client::Client() : s(), port(0), m_connected(false), m(), peers() {}

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

void Client::add_chat_text(const std::string &s) {
	std::lock_guard<std::mutex> lk(m_eng);
	printf("server says: \"%s\"\n", s.c_str());
	if (eng)
		eng->add_chat_text(s);
}

void Client::start_game() {
	std::lock_guard<std::mutex> lk(m_eng);
	puts("start game");
	if (eng)
		eng->trigger_multiplayer_started();
}

void Client::set_scn_vars(const ScenarioSettings &scn) {
	std::lock_guard<std::mutex> lk(m_eng);
	if (eng)
		eng->set_scn_vars(scn);
}

void Client::set_username(const std::string &s) {
	std::lock_guard<std::mutex> lk(m_eng);
	if (eng)
		eng->trigger_username(s);
}

void Client::playermod(const NetPlayerControl &ctl) {
	std::lock_guard<std::mutex> lk(m_eng);
	if (eng)
		eng->trigger_playermod(ctl);
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
				case NetPkgType::chat_text:
					add_chat_text(pkg.chat_text());
					break;
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
				default:
					printf("%s: type=%X\n", __func__, pkg.type());
					break;
			}
		}
	} catch (std::runtime_error &e) {
		fprintf(stderr, "%s: client stopped: %s\n", __func__, e.what());
	}

	std::lock_guard<std::mutex> lk(m_eng);
	if (eng) {
		eng->trigger_multiplayer_stop();
		if (!eng->is_hosting()) {
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
	pkg.set_chat_text(s);
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
