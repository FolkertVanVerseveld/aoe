#include "net.hpp"

#include "os_macros.hpp"

#include <cassert>
#include <cstring>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "endian.h"

#if windows
#include <io.h>

#define WSA_VERSION_MINOR 2
#define WSA_VERSION_MAJOR 2

#pragma comment(lib, "ws2_32.lib")

namespace genie {

int get_error() {
	return WSAGetLastError();
}

Net::Net() {
	WORD version = MAKEWORD(WSA_VERSION_MINOR, WSA_VERSION_MAJOR); // NOTE MAKEWORD(lo, hi)
	WSADATA wsa;
	int err;

	if ((err = WSAStartup(version, &wsa)) != 0)
		throw std::runtime_error(std::string("WSAStartup failed with error code ") + std::to_string(err));
}

Net::~Net() {
	int err = WSACleanup();
	assert(err == 0);
}

Socket::Socket() : port(0) {
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		throw std::runtime_error(std::string("Could not create TCP socket: code ") + std::to_string(WSAGetLastError()));
}

Socket::~Socket() {
	close();
}

void sock_block(SOCKET fd, bool enabled=true) {
	u_long iMode = !enabled;
	int err;
	if ((err = ioctlsocket(fd, FIONBIO, &iMode)) != 0)
		throw std::runtime_error(std::string("Could not set TCP blocking mode: code ") + std::to_string(err));
}

void Socket::close() {
	if (fd == INVALID_SOCKET)
		return;

	closesocket(fd);
	fd = INVALID_SOCKET;
}

void ServerSocket::eventloop(ServerCallback &cb) {
	SOCKET sock;
	sockaddr_in addr;
	int addrlen = sizeof addr;

	activated.store(true);

	while (activated.load()) {
		int err, incoming = 0;

		while ((sock = accept(this->sock.fd, (sockaddr*)&addr, &addrlen)) != INVALID_SOCKET) {
			sock_block(sock, false);

			WSAPOLLFD ev = { 0 };
			ev.fd = sock;
			ev.events = POLLRDNORM;

			peers.push_back(ev);
			cb.incoming(ev);
			++incoming;
		}
		if (incoming)
			printf("accepted %d socket%s\n", incoming, incoming == 1 ? "" : "s");

		if ((err = WSAGetLastError()) != WSAEWOULDBLOCK) {
			fprintf(stderr, "accept: %d\n", err);
			activated.store(false);
			continue;
		}

		// WSAPoll does not work properly if there are no peers, so check if we have to wait for any connections to arrive
		if (peers.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		int events;

		if ((events = WSAPoll(peers.data(), peers.size(), 500)) < 0) {
			fprintf(stderr, "poll failed: code %d\n", WSAGetLastError());
			activated.store(false);
			continue;
		}

		if (!events)
			continue;

		printf("poll: got %d event%s\n", events, events == 1 ? "" : "s");

		// choose which peers we want to keep
		std::vector<WSAPOLLFD> keep;

		for (unsigned i = 0; i < peers.size() && events; ++i) {
			auto ev = &peers[i];

			// drop any invalid sockets
			if (ev->revents & (POLLERR | POLLHUP | POLLNVAL)) {
				printf("drop event %u\n", i);
				--events;
				cb.removepeer(ev->fd);
				continue;
			}

			// TODO process pending data
			if (ev->revents & POLLRDNORM) {
				char dummy[256];
				int err, n;

				// FIXME use while loop
				if ((n = recv(ev->fd, dummy, sizeof dummy, 0)) == SOCKET_ERROR && (err = WSAGetLastError()) != WSAEWOULDBLOCK) {
					printf("drop event %u: code %d\n", i, err);
					--events;
					cb.removepeer(ev->fd);
					continue;
				}

				printf("read %d bytes from event %u\n", n, i);
			}
			else if (ev->revents) {
				printf("drop event %u: bogus state %d\n", i, ev->revents);
				--events;
				cb.removepeer(ev->fd);
				continue;
			}

			keep.push_back(*ev);
		}

		peers.swap(keep);
	}

	for (unsigned i = 0; i < peers.size(); ++i)
		closesocket(peers[i].fd);

	cb.shutdown();
}

void ServerSocket::close() {
	if (activated.load()) {
		activated.store(false);
		WSACancelBlockingCall(); // the calls we are using are probably non-blocking, but just be safe
	}
}

}
#elif linux
#include <cerrno>
#include <cstring>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define INVALID_SOCKET ((int)-1)

namespace genie {

Net::Net() {}
Net::~Net() {}

Socket::Socket() {
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		throw std::runtime_error(std::string("Could not create TCP socket: ") + strerror(errno));
}

Socket::~Socket() {
	close();
}

void Socket::close() {
	if (fd != -1) {
		::close(fd);
		fd = -1;
	}
}

void sock_block(int fd, bool enabled=true) {
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		throw std::runtime_error(std::string("Could not query TCP blocking mode: ") + strerror(errno));

	if (enabled)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;

	if (fcntl(fd, F_SETFL, flags) == -1)
		throw std::runtime_error(std::string("Could not adjust TCP blocking mode: ") + strerror(errno));
}

int get_error() {
	return errno;
}

void ServerSocket::close() {
	// XXX send cancellation request?
	if (efd != -1) {
		::close(efd);
		efd = -1;
	}
	// TODO figure out if this may trigger UB and/or leak memory
}

static constexpr unsigned MAX_EVENTS = 2 * MAX_USERS;

void ServerSocket::incoming(ServerCallback &cb) {
	while (1) {
		struct sockaddr in_addr;
		int err = 0, infd;
		socklen_t in_len = sizeof in_addr;

		if ((infd = accept(sock.fd, &in_addr, &in_len)) == -1) {
			if (errno != EAGAIN && errno != EWOULDBLOCK)
				perror("accept");
			break;
		}

		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

		// setup incoming connection and drop if errors occur
		// XXX is getnameinfo still vulnerable?
		if (!getnameinfo(&in_addr, in_len,
				hbuf, sizeof hbuf,
				sbuf, sizeof sbuf,
				NI_NUMERICHOST | NI_NUMERICSERV))
			printf("incoming: fd %d from %s:%s\n", infd, hbuf, sbuf);
		else
			printf("incoming: fd %d from unknown\n", infd);

		bool good = false;
		int flags, val;

		// nonblock, reuse, keepalive
		if ((flags = fcntl(infd, F_GETFL, 0)) == -1)
			goto reject;

		flags |= O_NONBLOCK;

		if (fcntl(infd, F_SETFL, flags) == -1)
			goto reject;

		val = 1;
		if (setsockopt(infd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof val))
			goto reject;

		val = 1;
		if (setsockopt(infd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&val, sizeof val))
			goto reject;

		good = true;
reject:
		// check if all socket options are set properly
		if (!good) {
			fprintf(stderr, "incoming: reject fd %d: %s\n", infd, strerror(errno));
			::close(infd);
			continue;
		}

		// first register the event, then add the slave
		struct epoll_event ev = {0};

		ev.data.fd = infd;
		ev.events = EPOLLIN;

		bool ins;

		if (epoll_ctl(efd, EPOLL_CTL_ADD, infd, &ev) || !(ins = peers.emplace(infd).second)) {
			if (!ins)
				throw std::runtime_error("incoming: internal error: peer already added");

			perror("epoll_ctl");
			fprintf(stderr, "incoming: reject fd %d: %s\n", infd, strerror(errno));
			::close(infd);
			continue;
		}

		cb.incoming(ev);
	}
}

void ServerSocket::removepeer(ServerCallback &cb, int fd) {
	peers.erase(fd);
	::close(fd);
	cb.removepeer(fd);
}

#define EPE_INVALID 1
#define EPE_READ 2

static const char *epetbl[] = {
	"success",
	"invalid event",
	"read error",
};

int ServerSocket::event_process(ServerCallback &cb, pollev &ev) {
	// Filter invalid/error events
	if ((ev.events & (EPOLLERR | EPOLLHUP)) && !(ev.events & EPOLLIN))
		return EPE_INVALID;

	// Process incoming events
	int fd = ev.data.fd;
	if (sock.fd == fd) {
		incoming(cb);
		return 0;
	}

	while (1) {
		int err;
		ssize_t n;
		char buf[256];

		if ((n = read(fd, buf, sizeof buf)) < 0) {
			if (errno != EAGAIN && errno != EWOULDBLOCK) {
				fprintf(stderr,
					"event_process: read error fd %d: %s\n",
					fd, strerror(errno)
				);
				return EPE_READ;
			}
			break;
		} else if (!n) {
			printf("event_process: remote closed fd %d\n", fd);
			return EPE_READ;
		}

		// TODO process data
		printf("read %zd bytes from fd %d\n", n, fd);

		auto search = rbuf.find(fd);

		if (search == rbuf.end()) {
			auto ins = rbuf.emplace(fd);
			assert(ins.second);
			search = ins.first;
		}

		if ((const_cast<CmdBuf&>(*search)).read(cb, buf, (unsigned)n)) {
			fprintf(stderr, "event_process: read buffer error fd %d\n", fd);
			return EPE_INVALID;
		}
	}

	return 0;
}

void ServerSocket::eventloop(ServerCallback &cb) {
	int dt = -1;
	epoll_event events[MAX_EVENTS];

	activated.store(true);

	while (activated.load()) {
		int err, n;

		// wait for new events
		if ((n = epoll_wait(efd, events, MAX_EVENTS, dt)) == -1) {
			/*
			 * This case occurs only when the server itself has been
			 * suspended and resumed. We can just ignore this case.
			 */
			if (errno == EINTR) {
				fputs("event_loop: interrupted\n", stderr);
				continue;
			}

			perror("epoll_wait");
			activated.store(false);
			continue;
		}

		for (int i = 0; i < n; ++i)
			if ((err = event_process(cb, events[i]))) {
				fprintf(stderr, "event_process: bad event (%d,%d): %s: %s\n", i, events[i].data.fd, epetbl[err], strerror(errno));
				removepeer(cb, events[i].data.fd);
			}

		// TODO determine new value
		dt = -1;
	}

	cb.shutdown();
}


}
#else
#error stub
#endif

namespace genie {

enum CmdType {
	TEXT,
};

void CmdData::hton() {}
void CmdData::ntoh() {}

void Command::hton() {
	type = htobe16(type);
	length = htobe16(length);
	data.hton();
}

void Command::ntoh() {
	type = be16toh(type);
	length = be16toh(length);
	data.ntoh();
}

std::string Command::text() {
	assert(type == CmdType::TEXT);
	data.text[TEXT_LIMIT - 1] = '\0';
	return data.text;
}

Command Command::text(const std::string& str) {
	Command cmd;

	cmd.type = CmdType::TEXT;
	cmd.length = TEXT_LIMIT;

	strncpy(cmd.data.text, str.c_str(), cmd.length);

	return cmd;
}

Socket::Socket(uint16_t port) : Socket() {
	this->port = port;
}

void Socket::reuse(bool enabled) {
	int val = enabled;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof val))
		throw std::runtime_error(std::string("Could not reuse TCP Socket: code ") + std::to_string(get_error()));
}

void Socket::bind() {
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(port);

	if (::bind(fd, (struct sockaddr*)&sa, sizeof sa))
		throw std::runtime_error(std::string("Could not bind TCP socket: code ") + std::to_string(get_error()));
}

int Socket::connect() {
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sa.sin_port = htons(port);

	return ::connect(fd, (struct sockaddr*)&sa, sizeof sa);
}

void Socket::listen() {
	if (::listen(fd, SOMAXCONN))
		throw std::runtime_error(std::string("Could not listen with TCP socket: code ") + std::to_string(get_error()));
}

void Socket::block(bool enabled) {
	sock_block(fd, enabled);
}

int Socket::send(const void* buf, unsigned len) {
	return ::send(fd, (const char*)buf, len, 0);
}

int Socket::recv(void* buf, unsigned len) {
	return ::recv(fd, (char*)buf, len, 0);
}

void Socket::sendFully(const void *buf, unsigned len) {
	for (unsigned sent = 0, rem = len; rem;) {
		int out;

		if ((out = send((const char*)buf + sent, rem)) <= 0)
			throw std::runtime_error(std::string("Could not send TCP data: code ") + std::to_string(get_error()));

		sent += out;
		rem -= out;
	}
}

void Socket::recvFully(void* buf, unsigned len) {
	for (unsigned got = 0, rem = len; rem;) {
		int in;

		if ((in = recv((char*)buf + got, rem)) <= 0)
			throw std::runtime_error(std::string("Could not receive TCP data: code ") + std::to_string(get_error()));

		got += in;
		rem -= in;
	}
}

void Socket::send(Command& cmd, bool net_order) {
	if (!net_order)
		cmd.hton();
	sendFully((const void*)&cmd, cmd.size());
}

ServerSocket::ServerSocket(uint16_t port)
	: sock(port)
#if linux
	, efd(-1)
#endif
	, peers(), rbuf(), activated(false)
{
	sock.reuse();
	sock.block(false);
	sock.bind();
	sock.listen();

#if linux
	if ((efd = epoll_create1(0)) == -1)
		throw std::runtime_error(std::string("Could not create epoll interface: ") + strerror(errno));

	struct epoll_event ev = {0};

	ev.data.fd = sock.fd;
	ev.events = EPOLLIN;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock.fd, &ev))
		throw std::runtime_error(std::string("Could not activate epoll interface: ") + strerror(errno));
#endif
}

ServerSocket::~ServerSocket() {
	close();
}

CmdBuf::CmdBuf(sockfd fd) : size(0), transmitted(0), endpoint(INVALID_SOCKET) {}

int CmdBuf::read(ServerCallback &cb, char *buf, unsigned len) {
	#if 0
	unsigned processed = 0;
	bool check_hdr = false;

	// FIXME use while loop
	// wait for complete header before processing any data
	if (size < CMD_HDRSZ) {
		unsigned need = CMD_HDRSZ - size;

		if (need > len) {
			memcpy((char*)&cmd + size, buf, len);
			size += len;
			return 0; // stop, we need more data
		}

		check_hdr = true;
	}

	// don't overflow the buffer
	unsigned available = sizeof cmd - size;

	processed = std::min(available, len);
	memcpy((char*)&cmd + size, buf + size, processed);

	size += processed;

	return 0;
	#else
	while (len) {
		unsigned need;

		// wait for complete header before processing any data
		if (transmitted < CMD_HDRSZ) {
			puts("waiting for header data");
			need = CMD_HDRSZ - transmitted;

			if (need > len) {
				memcpy((char*)&cmd + transmitted, buf, len);
				transmitted += len;
				return 0; // stop, we need more data
			}

			puts("got header");

			memcpy((char*)&cmd + transmitted, buf, need);
			transmitted += need;
			len -= need;

			// determine how big the complete packet is
			assert(be16toh(cmd.length) == TEXT_LIMIT);
			size = CMD_HDRSZ + be16toh(cmd.length);

			printf("trans, len, size: %u, %u, %u\n", transmitted, len, size);
		}

		assert(size);

		printf("need %u bytes, got %u\n", size, transmitted);

		// only process full packets
		if (transmitted >= size) {
			cmd.ntoh();

			printf("process: %d, %d\n", transmitted, size);

			cb.event_process(endpoint, cmd);

			// move pending data to front
			memmove((char*)&cmd, (char*)&cmd + size, transmitted - size);

			// update state
			len -= size;
			transmitted -= size;
			size = 0;
			continue; // process next packet
		}

		// packet incomplete, read only as much as we need
		need = size - transmitted;
		printf("need %u more bytes\n", need);
		assert(need);

		unsigned copy = std::min(need, len);

		memcpy((char*)&cmd + transmitted, buf, need);
		transmitted += copy;
		len -= copy;
		printf("read %u bytes: trans, len: %u, %u", need, transmitted, len);
	}
	#endif
}

bool operator<(const CmdBuf &lhs, const CmdBuf &rhs) {
	return lhs.endpoint < rhs.endpoint;
}

}
