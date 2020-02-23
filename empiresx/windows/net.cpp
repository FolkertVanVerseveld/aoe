#include "../base/net.hpp"

#include "../os_macros.hpp"
#include "../string.hpp"

#include <cassert>
#include <cstring>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>
#include <type_traits>

#include <io.h>

#define WSA_VERSION_MINOR 2
#define WSA_VERSION_MAJOR 2

#pragma comment(lib, "ws2_32.lib")

namespace genie {

int net_get_error() {
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

ServerSocket::ServerSocket(uint16_t port)
	: sock(port)
	, peers()
	, keep()
	, poke_peers(false)
	, rbuf(), wbuf(), activated(false), mut()
{
	sock.reuse();
	sock.block(false);
	sock.bind();
	sock.listen();
}

void ServerSocket::removepeer(ServerCallback &cb, SOCKET fd) {
	std::lock_guard<std::recursive_mutex> lock(mut);

	// remove slave from caches
	auto search = rbuf.find(fd);
	if (search != rbuf.end())
		rbuf.erase(search);
	wbuf.erase(fd);

	// purge connection
	closesocket(fd);
	// notify
	cb.removepeer(fd);
}

void ServerSocket::eventloop(ServerCallback &cb) {
	SOCKET sock;
	sockaddr_in addr;
	int addrlen = sizeof addr;

	activated.store(true);

	while (activated.load()) {
		int err, incoming = 0;

		// keep accepting any pending sockets
		{
			std::lock_guard<std::recursive_mutex> lock(mut);

			while ((sock = accept(this->sock.fd, (sockaddr*)&addr, &addrlen)) != INVALID_SOCKET) {
				sock_block(sock, false);

				WSAPOLLFD ev = {0};
				ev.fd = sock;
				ev.events = POLLRDNORM | POLLWRNORM;

				peers.push_back(ev);
				wbuf.emplace(std::make_pair(sock, std::queue<CmdBuf>()));
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
		}

		// WSAPoll does not work properly if there are no peers, so check if we have to wait for any connections to arrive
		if (peers.empty()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		// grab pending events
		int events;

		// (re)enable any sockets that we may have to write to
		if (poke_peers) {
			poke_peers = false;

			for (auto &x : peers)
				x.events |= POLLWRNORM;
		}

		if ((events = WSAPoll(peers.data(), (ULONG)peers.size(), 50)) < 0) {
			fprintf(stderr, "poll failed: code %d\n", WSAGetLastError());
			activated.store(false);
			continue;
		}

		if (!events)
			continue;

		printf("poll: got %d event%s\n", events, events == 1 ? "" : "s");

		// choose which peers we want to keep
		keep.clear();

		std::set<sockfd> handled;

		for (unsigned i = 0; i < peers.size() && events; ++i) {
			auto ev = &peers[i];

			handled.emplace(ev->fd);

			// drop any invalid sockets
			if (ev->revents & (POLLERR | POLLHUP | POLLNVAL)) {
				printf("drop event %u\n", i);
				--events;
				removepeer(cb, ev->fd);
				continue;
			}

			// process pending data
			if (ev->revents & POLLRDNORM) {
				char buf[TEXT_LIMIT];
				int err, n;

				// XXX use while loop
				// not strictly necessary to use while loop, since events are level triggered
				if ((n = recv(ev->fd, buf, sizeof buf, 0)) == SOCKET_ERROR && (err = WSAGetLastError()) != WSAEWOULDBLOCK) {
					printf("drop event %u: code %d\n", i, err);
					--events;
					removepeer(cb, ev->fd);
					continue;
				}

				printf("read %d bytes from event %u\n", n, i);

				std::lock_guard<std::recursive_mutex> lock(mut);
				auto search = rbuf.find(ev->fd);

				if (search == rbuf.end()) {
					auto ins = rbuf.emplace(ev->fd);
					assert(ins.second);
					search = ins.first;
				}

				if ((const_cast<CmdBuf&>(*search)).read(cb, buf, (unsigned)n)) {
					fprintf(stderr, "event_process: read buffer error fd %I64u\n", ev->fd);
					printf("drop event %u\n", i);
					--events;
					removepeer(cb, ev->fd);
					continue;
				}
			} else if (ev->revents & POLLWRNORM) {
				std::lock_guard<std::recursive_mutex> lock(mut);
				auto search = wbuf.find(ev->fd);
				assert(search != wbuf.end());

				if (!search->second.empty()) {
					switch (const_cast<CmdBuf&>(search->second.front()).write()) {
					case SSErr::PENDING: break;
					case SSErr::OK: search->second.pop(); break;
					default:
						fprintf(stderr, "event_process: write buffer error fd %I64u\n", ev->fd);
						printf("drop event %u\n", i);
						--events;
						removepeer(cb, ev->fd);
						continue;
					}
				}

				// disable write events if nothing to write (note that events are level triggered)
				if (search->second.empty())
					ev->events &= ~POLLWRNORM;
			} else if (ev->revents) {
				printf("drop event %u: bogus state %d\n", i, ev->revents);
				--events;
				removepeer(cb, ev->fd);
				continue;
			}

			keep.push_back(*ev);
		}

		// figure out which events weren't triggered
		for (auto &x : peers)
			if (handled.find(x.fd) == handled.end())
				keep.push_back(x);

		peers.swap(keep);
	}

	// XXX do we need to lock here?
	std::lock_guard<std::recursive_mutex> lock(mut);

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

SSErr ServerSocket::push(sockfd fd, const Command &cmd, bool net_order) {
	std::lock_guard<std::recursive_mutex> lock(mut);
	return push_unsafe(fd, cmd, net_order);
}

SSErr ServerSocket::push_unsafe(sockfd fd, const Command &cmd, bool net_order) {
	auto search = wbuf.find(fd);
	if (search == wbuf.end())
		return SSErr::BADFD;

	search->second.emplace(fd, cmd, net_order);
	poke_peers = true;

	return SSErr::OK;
}

bool str_to_ip(const std::string &str, uint32_t &ip) {
	in_addr addr;
	bool b = InetPtonW(AF_INET, utf8_to_wstring(str).c_str(), &addr) == 1;
	ip = addr.S_un.S_addr;
	return b;
}

}
