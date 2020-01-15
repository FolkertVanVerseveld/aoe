#pragma once

#include "os_macros.hpp"

#include <cstdint>

#include <vector>
#include <atomic>
#include <string>
#include <set>

#if windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <WinSock2.h>

typedef SOCKET sockfd;
typedef WSAPOLLFD pollev;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

typedef int sockfd;
typedef epoll_event pollev;
#endif

namespace genie {

class Net final {
public:
	Net();
	~Net();
};

static constexpr unsigned MAX_USERS = 64;

static constexpr unsigned TEXT_LIMIT = 256;

union CmdData final {
	char text[TEXT_LIMIT];

	void hton();
	void ntoh();
};

static constexpr unsigned CMD_HDRSZ = 4;

class CmdBuf;

class Command final {
	friend CmdBuf;

	uint16_t type, length;
	union CmdData data;
public:
	std::string text();

	unsigned size() const { return CMD_HDRSZ + length; }

	void hton();
	void ntoh();

	static Command text(const std::string &str);
};

class ServerCallback {
public:
	virtual void incoming(pollev& ev) = 0;
	virtual void removepeer(sockfd fd) = 0;
	virtual void shutdown() = 0;
	virtual void event_process(sockfd fd, Command &cmd) = 0;
};

class CmdBuf final {
	/** Total size in bytes and number of bytes read/written with the underlying socket. */
	unsigned size, transmitted;
	/** Communication device. */
	sockfd endpoint;
	/** The command to be read or sent in *network* byte endian order. */
	Command cmd;
public:
	CmdBuf(sockfd fd);

	int read(ServerCallback &cb, char *buf, unsigned len);

	friend bool operator<(const CmdBuf &lhs, const CmdBuf &rhs);
};

class ServerSocket;

class Socket final {
	friend ServerSocket;

	sockfd fd;
	uint16_t port;
public:
	/** Construct server accepted socket. If you want to specify the port (for e.g. bind, connect), you have to use the second ctor. */
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

class ServerSocket final {
	Socket sock;
#if linux
	int efd;
	std::set<int> peers;
#elif windows
	std::vector<pollev> peers;
#endif
	/** Cache for any pending read operations. */
	std::set<CmdBuf> rbuf; // FIXME probeer set want gare g++ errors
	// TODO add sockfd modification detection
	std::atomic<bool> activated;
public:
	ServerSocket(uint16_t port);
	~ServerSocket();

	void close();

#if linux
private:
	void removepeer(ServerCallback&, int fd);
	void incoming(ServerCallback&);
	int event_process(ServerCallback&, pollev &ev);
public:
#endif

	void eventloop(ServerCallback&);
};

}
