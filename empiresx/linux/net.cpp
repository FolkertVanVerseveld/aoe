#include "../base/net.hpp"

#include <cerrno>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../endian.h"

#include <cassert>
#include <stdexcept>
#include <string>

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

int net_get_error() {
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
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

		bool ins;

		if (epoll_ctl(efd, EPOLL_CTL_ADD, infd, &ev) || !(ins = peers.emplace(infd).second)) {
			if (!ins)
				throw std::runtime_error("incoming: internal error: peer already added");

			perror("epoll_ctl");
			fprintf(stderr, "incoming: reject fd %d: %s\n", infd, strerror(errno));
			::close(infd);
			continue;
		}

		wbuf.emplace(std::make_pair(infd, std::queue<CmdBuf>()));
		cb.incoming(ev);
	}
}

void ServerSocket::removepeer(ServerCallback &cb, int fd) {
	auto search = rbuf.find(fd);
	if (search != rbuf.end())
		rbuf.erase(search);
	wbuf.erase(fd);
	// purge connection
	peers.erase(fd);
	::close(fd);
	// notify
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
	if ((ev.events & (EPOLLERR | EPOLLHUP)) && !(ev.events & (EPOLLIN | EPOLLOUT)))
		return EPE_INVALID;

	// Process incoming events
	int fd = ev.data.fd;
	if (sock.fd == fd) {
		incoming(cb);
		return 0;
	}

	if (ev.events & EPOLLIN)
		while (1) {
			int err;
			ssize_t n;
			char buf[TEXT_LIMIT];

			printf("reading from fd %d...\n", fd);

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

	if (ev.events & EPOLLOUT) {
		auto search = wbuf.find(fd);
		assert(search != wbuf.end());

		while (!search->second.empty()) {
			switch (const_cast<CmdBuf&>(search->second.front()).write()) {
			case SSErr::PENDING: break;
			case SSErr::OK: search->second.pop(); break;
			default:
				fprintf(stderr, "event_process: write buffer error fd %d\n", fd);
				return EPE_INVALID;
			}
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

ServerSocket::ServerSocket(uint16_t port)
	: sock(port)
	, efd(-1)
	, peers()
	, rbuf(), wbuf(), activated(false)
{
	sock.reuse();
	sock.block(false);
	sock.bind();
	sock.listen();

	if ((efd = epoll_create1(0)) == -1)
		throw std::runtime_error(std::string("Could not create epoll interface: ") + strerror(errno));

	struct epoll_event ev = {0};

	ev.data.fd = sock.fd;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

	if (epoll_ctl(efd, EPOLL_CTL_ADD, sock.fd, &ev))
		throw std::runtime_error(std::string("Could not activate epoll interface: ") + strerror(errno));
}

bool str_to_ip(const std::string &str, uint32_t &ip) {
	in_addr addr;
	bool b = inet_aton(str.c_str(), &addr) == 1;
	ip = addr.s_addr;
	return b;
}

}
