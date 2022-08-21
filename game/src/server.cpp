#include "server.hpp"

#include "game.hpp"

namespace aoe {

Server::Server() : s(), m_running(false), m() {}

Server::Server(uint16_t port) : Server() {
	start(port);
}

Server::~Server() {
	stop();
}

void Server::start(uint16_t port) {
	std::lock_guard<std::mutex> lk(m);
	m_running = false;

	s.open("127.0.0.1", port);
	m_running = true;
}

int Server::mainloop(int) {
	SOCKET host;

	if ((host = s.accept()) == INVALID_SOCKET)
		return -1;

	TcpSocket peer(host);
	char buf[32];
	int in;

	// just echo anything the client sends for now
	while ((in = peer.recv(buf, sizeof buf)) > 0) {
		peer.send(buf, in);
	}

	return 0;
}

void Server::stop() {
	std::lock_guard<std::mutex> lk(m);
	m_running = false;
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
}

}
