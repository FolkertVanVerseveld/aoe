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

Socket::Socket(uint16_t port) : Socket() {
	this->port = port;
}

Socket::~Socket() {
	close();
}

void sock_block(sockfd fd, bool enabled=true) {
	u_long iMode = !enabled;
	int err;
	if ((err = ioctlsocket(fd, FIONBIO, &iMode)) != 0)
		throw std::runtime_error(std::string("Could not set TCP blocking mode: code ") + std::to_string(err));
}

void Socket::block(bool enabled) {
	sock_block(fd, enabled);
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

ServerSocket::ServerSocket(uint16_t port) : sock(port), activated(false) {
	sock.reuse();
	sock.block(false);
	sock.bind();
	sock.listen();
}

ServerSocket::~ServerSocket() {
	close();
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

namespace genie {

Net::Net() {}
Net::~Net() {}

Socket::Socket() {
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		throw std::runtime_error(std::string("Could not create TCP socket: ") + strerror(errno));
}

Socket::~Socket() {
	close(fd);
}

int get_error() {
	return errno;
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
	cmd.length = sizeof cmd.data.text;

	strncpy(cmd.data.text, str.c_str(), cmd.length);

	return cmd;
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

}
