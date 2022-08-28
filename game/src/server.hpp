#pragma once

#include "net.hpp"

#include <mutex>
#include <vector>

#if _WIN32
#include "../../wepoll/wepoll.h"
#endif

namespace aoe {

struct NetPkgHdr final {
	uint16_t type, payload;
	bool native_ordering;

	NetPkgHdr(uint16_t type, uint16_t payload) : type(type), payload(payload), native_ordering(true) {}

	// byte swapping
	void ntoh();
	void hton();
};

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;

	NetPkg(uint16_t type, uint16_t payload) : hdr(type, payload), data() {}

	void set_protocol(uint16_t version);
	uint16_t protocol_version();

	void ntoh();
	void hton();
};

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

	void send(NetPkg&);
	NetPkg recv();

	void send_protocol(uint16_t version);
};

}
