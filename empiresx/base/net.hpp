#pragma once

#include "../os_macros.hpp"

#include <cstdint>

#include <vector>
#include <atomic>
#include <string>
#include <set>
#include <map>
#include <queue>

#if windows
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

typedef SOCKET sockfd;
typedef WSAPOLLFD pollev;

static inline SOCKET pollfd(const pollev &ev) { return ev.fd; }
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <arpa/inet.h>
#include <netinet/in.h>

typedef int sockfd;
typedef epoll_event pollev;

static inline int pollfd(const epoll_event &ev) { return ev.data.fd; }
static inline int pollfd(int fd) { return fd; }
#endif

namespace genie {

int net_get_error();
bool str_to_ip(const std::string &str, uint32_t &addr);

class Net final {
public:
	Net();
	~Net();
};

static constexpr unsigned MAX_USERS = 64;
static constexpr unsigned TEXT_LIMIT = 32;

union CmdData final {
	char text[TEXT_LIMIT];

	void hton();
	void ntoh();
};

static constexpr unsigned CMD_HDRSZ = 4;

class CmdBuf;

enum CmdType {
	TEXT,
	MAX,
};

class Command final {
	friend CmdBuf;

public:
	uint16_t type, length;
	union CmdData data;
	std::string text();

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

enum class SSErr {
	OK,
	BADFD,
	PENDING,
	WRITE,
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
	CmdBuf(sockfd fd, const Command& cmd, bool net_order=false);

	int read(ServerCallback &cb, char *buf, unsigned len);
	/** Try to send the command completely. Zero is returned if the all data has been sent. */
	SSErr write();

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
	int connect(uint32_t addr, bool netorder=false);

	void close();

	int send(const void *buf, unsigned size);
	int recv(void *buf, unsigned size);

	/**
	 * Block until all data has been fully send.
	 * It is UB to call this if the socket is in non-blocking mode.
	 */
	void sendFully(const void *buf, unsigned len);
	void recvFully(void *buf, unsigned len);

	template<typename T> int send(const T &t) {
		return send((const void*)&t, sizeof t);
	}

	int recv(Command &cmd);

	void send(Command &cmd, bool net_order=false);
};

// XXX use locking to make it thread-safe
class ServerSocket final {
	Socket sock;
#if linux
	int efd;
	std::set<int> peers;
#elif windows
	std::vector<pollev> peers, keep;
	bool poke_peers;
#endif
	/** Cache for any pending read operations. */
	std::set<CmdBuf> rbuf;
	/** Cache for any pending write operations. */
	std::map<sockfd, std::queue<CmdBuf>> wbuf;
	std::atomic<bool> activated;
public:
	ServerSocket(uint16_t port);
	~ServerSocket();

	void close();

	SSErr push(sockfd fd, const Command &cmd, bool net_order=false);
	void broadcast(Command &cmd, bool net_order=false);

private:
	void removepeer(ServerCallback&, sockfd fd);
#if linux
	void incoming(ServerCallback&);
	int event_process(ServerCallback&, pollev &ev);
#endif
public:
	void eventloop(ServerCallback&);
};

}
