#pragma once

#include "os_macros.hpp"

#include <cstdint>

#include <vector>
#include <atomic>

#if windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <WinSock2.h>

typedef uintptr_t sockfd;
#else
typedef int sockfd;
#endif

namespace genie {

class Net final {
public:
	Net();
	~Net();
};

class ServerSocket;

class Socket final {
	friend ServerSocket;

	sockfd fd;
	uint16_t port;
public:
	Socket();
	Socket(uint16_t port);
	~Socket();

	void block(bool enabled=true);
	void reuse(bool enabled=true);

	void bind();
	void listen();
	int connect();

	void close();
};

class ServerSocket final {
	Socket sock;
	std::vector<WSAPOLLFD> peers;
	std::atomic<bool> activated;
public:
	ServerSocket(uint16_t port);
	~ServerSocket();

	void close();

	void eventloop();
};

}
