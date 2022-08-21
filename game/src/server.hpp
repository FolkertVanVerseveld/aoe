#pragma once

#include "net.hpp"

#include <mutex>

#if _WIN32
#include "../../wepoll/wepoll.h"
#endif

namespace aoe {

class Server final {
	ServerSocket s;
	std::atomic<bool> m_running;
	std::mutex m;
public:
	Server();
	~Server();

	Server(uint16_t port);

	void start(uint16_t port);
	void stop();
	void close(); // this will block till everything is stopped

	bool running() const noexcept { return m_running; }

	int mainloop(int id);
};

class Client final {
	TcpSocket s;
	std::string host;
	uint16_t port;
	std::atomic<bool> m_connected;
	std::mutex m;
public:
	Client();
	~Client();

	void start(const char *host, uint16_t port);
	void stop();

	bool connected() const noexcept { return m_connected; }

	template<typename T> void send(T *ptr, int len) {
		s.send_fully(ptr, len);
	}

	template<typename T> void recv(T *ptr, int len) {
		s.recv_fully(ptr, len);
	}
};

}
