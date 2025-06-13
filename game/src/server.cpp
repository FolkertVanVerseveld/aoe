#include "server.hpp"

#include "engine.hpp"
#include "engine/audio.hpp"

#include <string.hpp>

namespace aoe {

Server::Server()
	: ServerSocketController(), IServer()
	, s(), m_running(false), m_peers(), port(0), protocol(0), peers(), refs(), w(), civs() {}

Server::~Server() {
	stop();
}

struct pkg {
	uint16_t length;
	uint16_t type;
};

bool Server::incoming(ServerSocket &s, const Peer &p) {
	std::lock_guard<std::mutex> lk(m_peers);

	if (peers.size() > max_peers || m_running)
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

	pkg.set_peer_ref(ref);
	send(p, pkg);

	pkg.set_username(name);
	send(p, pkg);

	pkg.set_player_resize(w.scn.players.size());
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

	// prevent dangling owners
	w.scn.remove(ci.ref);

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

bool Server::chk_protocol(const Peer &p, std::deque<uint8_t> &out, NetPkg &in) {
	uint16_t req = in.protocol_version();
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

	// disallow name change while game is running
	if (m_running) {
		NetPkg pkg;
		pkg.set_username(old);
		pkg.write(out);

		return true;
	}

	printf("%s: (%s,%s) wants to change username from \"%s\" to \"%s\"\n", __func__, p.host.c_str(), p.server.c_str(), old.c_str(), name.c_str());

	// allow anything when it's unique and doesn't contain `:'. we use : ourselves when generating names
	if (contains(name, ':')) {
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

	NetPkg pkg;
	w.load_scn(scn);
	pkg.set_scn_vars(w.scn);
	broadcast(pkg);

	return true;
}

bool Server::process_packet(ServerSocket &s, const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed) {
	NetPkg pkg(in);
	return process(p, pkg, out);
}

int Server::mainloop(uint16_t port, uint16_t protocol, bool testing) {
	this->port = port;
	this->protocol = protocol;

	if (!testing) {
		Assets &a = eng->gamedata();

		civs = old_lang.civs;
		old_lang.collect_civs(civnames);
	}

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

}
