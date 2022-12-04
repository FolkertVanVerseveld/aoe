#include "server.hpp"

#include "engine.hpp"

namespace aoe {

Server::Server() : ServerSocketController(), s(), m_active(false), m_peers(), port(0), protocol(0), peers() {}

Server::~Server() {
	stop();
}

struct pkg {
	uint16_t length;
	uint16_t type;
};

void Server::incoming(ServerSocket &s, const Peer &p) {
	std::lock_guard<std::mutex> lk(m_peers);
	peers[p] = ClientInfo();

	NetPkg pkg;
	pkg.set_chat_text("peer joined");

	broadcast(pkg);
}

void Server::dropped(ServerSocket &s, const Peer &p) {
	std::lock_guard<std::mutex> lk(m_peers);
	peers.erase(p);

	NetPkg pkg;
	pkg.set_chat_text("peer left");

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

bool Server::chk_protocol(const Peer &p, std::deque<uint8_t> &out, uint16_t req) {
	printf("%s: (%s,%s) requests protocol %u. answer protocol %u\n", __func__, p.host.c_str(), p.server.c_str(), req, protocol);

	NetPkg pkg;
	pkg.set_protocol(protocol);
	pkg.write(out);

	return true;
}

bool Server::process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out) {
	pkg.ntoh();

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
			if (p.is_host) {
				broadcast(pkg, false);
			} else {
				// TODO check if restricted is disabled

				// if restricted, client is modded: kick now!
				fprintf(stderr, "hacked client %s:%s: kick!\n", p.host.c_str(), p.server.c_str());
				return false;
			}
			break;
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

Client::Client() : s(), port(0), m_connected(false), m() {}

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

void Client::send_scn_vars(const ScenarioSettings &scn) {
	printf("%s: %u,%u\n", __func__, scn.width, scn.height);
	NetPkg pkg;
	pkg.set_scn_vars(scn);
	send(pkg);
}

}
