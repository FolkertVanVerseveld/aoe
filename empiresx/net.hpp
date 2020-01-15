#pragma once

#include "os_macros.hpp"

#include <cstdint>

#include <vector>
#include <atomic>
#include <string>

#if windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <WinSock2.h>

typedef SOCKET sockfd;
#else
typedef int sockfd;
#endif

namespace genie {

class Net final {
public:
	Net();
	~Net();
};

static constexpr unsigned TEXT_LIMIT = 256;

union CmdData final {
	char text[TEXT_LIMIT];

	void hton();
	void ntoh();
};

class Command final {
	uint16_t type, length;
	union CmdData data;
public:
	std::string text();

	unsigned size() const { return 4 + length; }

	void hton();
	void ntoh();

	static Command text(const std::string &str);
};

class ServerSocket;

class Socket final {
	friend ServerSocket;

	sockfd fd;
	uint16_t port;
public:
	/** Construct server accepted socket. If you want to specify the port, you have to use the second ctor. */
	Socket();
	Socket(uint16_t port);
	~Socket();

	void block(bool enabled=true);
	void reuse(bool enabled=true);

	void bind();
	void listen();
	int connect();

	void close();

	int send(const void* buf, unsigned size);
	int recv(void* buf, unsigned size);

	/**
	 * Block until all data has been fully send.
	 * It is UB to call this if the socket is in non-blocking mode.
	 */
	void sendFully(const void* buf, unsigned len);
	void recvFully(void* buf, unsigned len);

	template<typename T> int send(const T& t) {
		return send((const void*)&t, sizeof t);
	}

	template<typename T> int recv(T& t) {
		return recv((void*)&t, sizeof t);
	}

	template<typename T> void sendFully(const T& t) {
		sendFully(&t, sizeof t);
	}

	template<typename T> void recvFully(T& t) {
		recvFully(&t, sizeof t);
	}

	void send(Command& cmd, bool net_order=false);
};

class ServerCallback {
public:
	virtual void incoming(WSAPOLLFD& ev) = 0;
	virtual void removepeer(sockfd fd) = 0;
	virtual void shutdown() = 0;
};

class ServerSocket final {
	Socket sock;
	std::vector<WSAPOLLFD> peers;
	std::atomic<bool> activated;
public:
	ServerSocket(uint16_t port);
	~ServerSocket();

	void close();

	void eventloop(ServerCallback&);
};

}
