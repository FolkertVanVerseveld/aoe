#include "server.hpp"

#include "game.hpp"

namespace aoe {

Server::Server() : s(), port(0), m_running(false), m() {}

Server::Server(uint16_t port) : Server() {
	start(port);
}

Server::~Server() {
	stop();
}

void Server::start(uint16_t port) {
	std::lock_guard<std::mutex> lk(m);

	m_running = false;

	s.bind("127.0.0.1", port);
	s.listen(max_players);
	this->port = port;

	m_running = true;
}

int Server::mainloop(int) {
	s.accept();
	return 0;
}

void Server::stop() {
	std::lock_guard<std::mutex> lk(m);
	m_running = false;
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
	s.close();
	m_connected = false;

	s.connect(host, port);
	m_connected = true;
}

}
