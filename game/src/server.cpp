#include "server.hpp"

#include "game.hpp"

namespace aoe {

Server::Server() : s(), m_active(false), m(), port(0), protocol(0), peers() {}

Server::~Server() {
	stop();
}

struct pkg {
	uint16_t length;
	uint16_t type;
};

static int pkg_check_proper(const std::deque<uint8_t> &q) {
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
		default:
			throw "invalid type";
	}

	return true;
}

static bool pkg_process(const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int, void *arg) {
	NetPkg pkg(in);
	Server &s = *((Server*)arg);
	return s.process(p, pkg, out);
}

int Server::mainloop(int, uint16_t port, uint16_t protocol) {
	this->port = port;
	this->protocol = protocol;

	m_active = true;
	int r = s.mainloop(port, 10, pkg_check_proper, pkg_process, this);

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
	s.close();
	m_connected = false;
}

void Client::start(const char *host, uint16_t port) {
	std::lock_guard<std::mutex> lk(m);
	s.open();
	m_connected = false;

	s.connect(host, port);
	m_connected = true;

	send_protocol(1);
	printf("prot=%u\n", recv_protocol());
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

}
