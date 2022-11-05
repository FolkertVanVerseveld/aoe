#include "net.hpp"

#include <cstdio>
#include <cassert>

#include <atomic>
#include <string>

#include "nominmax.hpp"

#if _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#endif

#include <tracy/Tracy.hpp>

// TODO merge common errors

namespace aoe {

// process and throw error message. always throws
static void wsa_generic_error(const char *prefix, int code) noexcept(false)
{
	std::string msg(prefix);
	msg += ": ";

	switch (code) {
		case WSANOTINITIALISED:
			msg += "winsock not ready";
			break;
		case WSAENETDOWN:
			msg += "network subsystem error";
			break;
		case WSAENOBUFS:
			msg += "out of memory";
			break;
		case WSAENOTSOCK:
			msg += "invalid socket";
			break;
		case WSAEOPNOTSUPP:
			msg += "operation not supported";
			break;
		default:
			msg += "code " + std::to_string(code);
			break;
	}

	throw std::runtime_error(msg);
}

std::atomic<unsigned> initnet(0);

void set_nonblocking(SOCKET s, bool nonbl) {
	u_long arg = !!nonbl;
	if (!ioctlsocket(s, FIONBIO, &arg))
		return;

	int r;

	switch (r = WSAGetLastError()) {
		case WSAEFAULT:
			throw std::runtime_error("wsa: argument address fault");
		default:
			wsa_generic_error("wsa: cannot change non-blocking mode", r);
			break;
	}
}

Net::Net() {
	WORD version = MAKEWORD(2, 2);
	WSADATA wsa = { 0 };

	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
	int r = WSAStartup(version, &wsa);
	switch (r) {
		case WSASYSNOTREADY:
			throw std::runtime_error("wsa: winsock not ready");
		case WSAVERNOTSUPPORTED:
			throw std::runtime_error("wsa: winsock version not supported");
		case WSAEINPROGRESS:
			throw std::runtime_error("wsa: winsock blocked");
			break;
		case WSAEPROCLIM:
			throw std::runtime_error("wsa: winsock process limit reached");
			break;
		default:
			if (r)
				throw std::runtime_error(std::string("wsa: winsock error code ") + std::to_string(r));
			break;
	}

	++initnet;
}

Net::~Net() {
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
	int r = WSACleanup();

	if (r == SOCKET_ERROR)
		// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsacleanup
		r = WSAGetLastError();

	switch (r) {
		case WSANOTINITIALISED: fprintf(stderr, "%s: winsock not initialised\n", __func__); break;
		case WSAENETDOWN: fprintf(stderr, "%s: winsock failed\n", __func__); break;
		case WSAEINPROGRESS: fprintf(stderr, "%s: winsock is blocked\n", __func__); break;
	}

	--initnet;
}

TcpSocket::TcpSocket() : s(INVALID_SOCKET) {
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		throw std::runtime_error("socket failed");
}

TcpSocket::~TcpSocket() {
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-closesocket
	closesocket(s);
}

void TcpSocket::open() {
	close();
	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		throw std::runtime_error("socket failed");
}

void TcpSocket::close() {
	if (!closesocket(s))
		s = INVALID_SOCKET;
}

SOCKET TcpSocket::accept() {
	return ::accept(s, NULL, NULL);
}

SOCKET TcpSocket::accept(sockaddr &a, int &sz) {
	return ::accept(s, &a, &sz);
}

void TcpSocket::bind(const char *address, uint16_t port) {
	sockaddr_in dst{ 0 };

	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = inet_addr(address);
	dst.sin_port = htons(port);

	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
	int r = ::bind(s, (const sockaddr *)&dst, sizeof dst);

	if (r == 0)
		return;

	if (r != SOCKET_ERROR)
		throw std::runtime_error(std::string("wsa: bind failed: unknown return code ") + std::to_string(r));

	int err = WSAGetLastError();

	switch (err) {
		case WSAEACCES:
			throw std::runtime_error("wsa: bind failed: access denied");
		case WSAEADDRINUSE:
			throw std::runtime_error("wsa: bind failed: socket address still in use");
		case WSAEADDRNOTAVAIL:
			throw std::runtime_error("wsa: bind failed: invalid address");
		case WSAEFAULT:
			throw std::runtime_error("wsa: bind failed: bad argument");
		case WSAEINPROGRESS:
			throw std::runtime_error("wsa: bind failed: in progress");
		case WSAEINVAL:
			throw std::runtime_error("wsa: bind failed: already bound");
		default:
			wsa_generic_error("wsa: bind failed", err);
			break;
	}
}

void TcpSocket::listen(int backlog) {
	if (backlog < 1)
		throw std::runtime_error("listen failed: backlog must be positive");

	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	int r = ::listen(s, backlog);

	if (r == 0)
		return;

	if (r != SOCKET_ERROR)
		throw std::runtime_error(std::string("wsa: listen failed: unknown return code ") + std::to_string(r));

	int err = WSAGetLastError();

	switch (err) {
		case WSANOTINITIALISED:
			throw std::runtime_error("wsa: listen failed: winsock not ready");
		case WSAENETDOWN:
			throw std::runtime_error("wsa: listen failed: network subsystem error");
		case WSAEADDRINUSE:
			throw std::runtime_error("wsa: listen failed: socket address still in use");
		case WSAEINPROGRESS:
			throw std::runtime_error("wsa: listen failed: in progress");
		case WSAEINVAL:
			throw std::runtime_error("wsa: listen failed: socket not bound");
		case WSAEISCONN:
			throw std::runtime_error("wsa: listen failed: socket is connected and cannot listen");
		case WSAEMFILE:
			throw std::runtime_error("wsa: listen failed: out of socket resources");
		default:
			throw std::runtime_error(std::string("wsa: listen failed: code ") + std::to_string(err));
	}
}

void TcpSocket::connect(const char *address, uint16_t port) {
	(void)address;
	(void)port;

	sockaddr_in dst{ 0 };

	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = inet_addr(address);
	dst.sin_port = htons(port);

	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
	int r = ::connect(s, (const sockaddr *)&dst, sizeof dst);

	if (r == 0)
		return;

	if (r != SOCKET_ERROR)
		throw std::runtime_error(std::string("wsa: connect failed: unknown return code ") + std::to_string(r));

	int err = WSAGetLastError();

	switch (err) {
		case WSAEADDRINUSE:
			throw std::runtime_error("wsa: connect failed: socket address still in use");
		case WSAEINTR:
			throw std::runtime_error("wsa: connect failed: cancelled");
		case WSAEINPROGRESS:
			throw std::runtime_error("wsa: connect failed: in progress");
		case WSAEALREADY:
			throw std::runtime_error("wsa: connect failed: already connecting asynchronously");
		case WSAEADDRNOTAVAIL:
			throw std::runtime_error("wsa: connect failed: invalid address");
		case WSAEAFNOSUPPORT:
			throw std::runtime_error("wsa: connect failed: operation not supported");
		case WSAECONNREFUSED:
			throw std::runtime_error("wsa: connect failed: cancelled by destination");
		case WSAEFAULT:
			throw std::runtime_error("wsa: connect failed: bad argument");
		case WSAEINVAL:
			throw std::runtime_error("wsa: connect failed: socket in bound or listening mode");
		case WSAEISCONN:
			throw std::runtime_error("wsa: connect failed: already connected");
		case WSAENETUNREACH:
			throw std::runtime_error("wsa: connect failed: network unreachable");
		case WSAEHOSTUNREACH:
			throw std::runtime_error("wsa: connect failed: host unreachable");
		case WSAENOTSOCK:
			throw std::runtime_error("wsa: connect failed: invalid socket");
		case WSAETIMEDOUT:
			throw std::runtime_error("wsa: connect failed: cancelled by timeout");
		case WSAEWOULDBLOCK:
			throw std::runtime_error("wsa: connect failed: connect will block: try again later");
		case WSAEACCES:
			throw std::runtime_error("wsa: connect failed: access denied");
		default:
			wsa_generic_error("wsa: connect failed", err);
			break;
	}
}

int TcpSocket::try_send(const void *ptr, int len, unsigned tries) noexcept {
	int written = 0;

	while (written < len) {
		int rem = len - written;
		int out = ::send(s, (const char *)ptr + written, rem, 0);

		if (out <= 0) {
			if (!written)
				return out; // probably an error
			break;
		}

		written += out;

		if (tries && !--tries)
			break;
	}

	return written;
}

int TcpSocket::send(const void *ptr, int len, unsigned tries) {
	int out = try_send(ptr, len, tries);

	if (out != SOCKET_ERROR && out >= 0)
		return out;

	if (out < 0)
		throw std::runtime_error(std::string("wsa: send failed: unknown return code ") + std::to_string(out));

	int err = WSAGetLastError();

	switch (err) {
		default:
			wsa_generic_error("wsa: send failed", err);
			break;
	}

	return out;
}

void TcpSocket::send_fully(const void *ptr, int len) {
	int out = send(ptr, len, 0);
	if (out != len) {
		if (out < 0)
			out = 0;
		throw std::runtime_error(std::string("tcp: send_fully failed: ") + std::to_string(out) + (out == 1 ? " byte written out of " : " bytes written out of ") + std::to_string(len));
	}
}

int TcpSocket::try_recv(void *dst, int len, unsigned tries) noexcept {
	int read = 0;

	while (read < len) {
		int rem = len - read;
		int in = ::recv(s, (char *)dst + read, rem, 0);

		if (in <= 0) {
			if (!read)
				return in; // probably an error
			break;
		}

		read += in;

		if (tries && !--tries)
			break;
	}

	return read;
}

int TcpSocket::recv(void *dst, int len, unsigned tries) {
	int in = try_recv(dst, len, tries);

	if (in != SOCKET_ERROR && in >= 0)
		return in;

	if (in < 0)
		throw std::runtime_error(std::string("wsa: recv failed: unknown return code ") + std::to_string(in));

	int err = WSAGetLastError();

	switch (err) {
		default:
			wsa_generic_error("wsa: recv failed", err);
			break;
	}

	return in;
}

void TcpSocket::recv_fully(void *ptr, int len) {
	int in;

	if ((in = recv(ptr, len, 0)) == len)
		return;

	if (in < 0)
		in = 0;
	throw std::runtime_error(std::string("tcp: recv_fully failed: ") + std::to_string(in) + (in == 1 ? " byte read out of " : " bytes read out of ") + std::to_string(len));
}

ServerSocket::ServerSocket() : s(), h(INVALID_HANDLE_VALUE), port(0), events(), peers(), peer_host(INVALID_SOCKET), data_lock(), data_in(), data_out(), running(false), proper_packet(nullptr), process_packet(nullptr), process_arg(nullptr) {}

ServerSocket::~ServerSocket() { stop(); }

void ServerSocket::open(const char *addr, uint16_t port, unsigned backlog) {
	ZoneScoped;
	s.bind(addr, port);
	s.listen(backlog);
}

void ServerSocket::stop() {
	ZoneScoped;
	for (const epoll_event &ev : events) {
		SOCKET s = ev.data.sock;
		::closesocket(s);
	}

	events.clear();
	peers.clear();

	if (h != INVALID_HANDLE_VALUE)
		if (!epoll_close(h))
			h = INVALID_HANDLE_VALUE;

	running = false;
}

void ServerSocket::close() {
	ZoneScoped;
	stop();
	s.close();
}

int ServerSocket::add_fd(SOCKET s) {
	ZoneScoped;
	epoll_event ev{ 0 };
	ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP;
	ev.data.fd = s;

	return epoll_ctl(h, EPOLL_CTL_ADD, s, &ev);
}

void ServerSocket::incoming() {
	ZoneScoped;

	while (1) {
		sockaddr in_addr;
		int in_len;
		SOCKET infd;
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

		hbuf[0] = sbuf[0] = '\0';

		if ((infd = s.accept(in_addr, in_len = sizeof in_addr)) == INVALID_SOCKET) {
			int r = WSAGetLastError();

			if (r == WSAEINPROGRESS || r == WSAEWOULDBLOCK)
				return; // all peers have been accepted. stop

			wsa_generic_error("ssock incoming", r);
			return;
		}

		if (getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV)) {
			// force drop
			::closesocket(infd);
			continue;
		}

		// set up peer
		printf("%s: descriptor %u: %s:%s\n", __func__, (unsigned)infd, hbuf, sbuf);
		set_nonblocking(infd);
		if (add_fd(infd))
			throw std::runtime_error("ssock: add_fd failed");
		auto ins = peers.emplace(std::piecewise_construct, std::forward_as_tuple(infd), std::forward_as_tuple(hbuf, sbuf));
		assert(ins.second);

		// check if peer is host
		const Peer &p = ins.first->second;
		if (p.host == "127.0.0.1") {
			printf("%s: host joined at service %s\n", __func__, sbuf);
			peer_host = infd;
		}
	}
}

bool ServerSocket::io_step(int idx) {
	ZoneScoped;

	epoll_event &ev = events.at(idx);
	SOCKET s = ev.data.sock;

	auto it = peers.find(s);
	if (it == peers.end())
		return false;

	return recv_step(it->second, s) && send_step(s);
}

bool ServerSocket::recv_step(const Peer &p, SOCKET s) {
	ZoneScoped;
	std::unique_lock<std::mutex> lk(data_lock, std::defer_lock);

	while (1) {
		int count;
		char buf[512];

		count = ::recv(s, buf, sizeof buf, 0);
		if (count < 0) {
			int r = WSAGetLastError();

			if (r != WSAEWOULDBLOCK) {
				fprintf(stderr, "%s: recv failed: code %d\n", __func__, r);
				return false;
			}

			return true;
		}

		if (count == 0)
			return false; // peer send shutdown request or has closed socket

		lk.lock();

		auto ins = data_in.try_emplace(s);
		auto it = ins.first;
		for (int i = 0; i < count; ++i)
			it->second.emplace_back(buf[i]);

		int processed = proper_packet(it->second);

		// remove bytes if asked to do so
		for (; processed < 0 && !it->second.empty(); ++processed)
			it->second.pop_front();

		// skip processing if packet is improper
		if (processed < 1) {
			lk.unlock();
			continue;
		}

		assert(proper_packet(it->second) > 0);

		auto outs = data_out.try_emplace(s);
		auto out = outs.first;

		bool keep_alive = false;

		try {
			keep_alive = process_packet(p, it->second, out->second, processed, process_arg);
		} catch (const std::runtime_error &e) {
			fprintf(stderr, "%s: failed to process for (%s,%s): %s\n", __func__, p.host.c_str(), p.server.c_str(), e.what());
		}

		lk.unlock();

		if (!keep_alive)
			return false;
	}
}

bool ServerSocket::send_step(SOCKET s) {
	ZoneScoped;
	std::unique_lock<std::mutex> lk(data_lock);

	auto it = data_out.find(s);
	if (it == data_out.end())
		return false;

	auto &q = it->second;
	while (!q.empty()) {
		int count;
		char buf[1024];
		int out = (int)std::min(sizeof buf, q.size());

		for (int i = 0; i < out; ++i)
			buf[i] = q[i];

		// s may be closed after this unlock, but this way, we give a brief moment for other threads to kick in
		lk.unlock();

		count = ::send(s, buf, out, 0);
		if (count < 0) {
			int r = WSAGetLastError();

			if (r != WSAEWOULDBLOCK) {
				fprintf(stderr, "%s: send failed: code %d\n", __func__, r);
				return false;
			}

			return true;
		}

		if (count == 0)
			return false; // peer send shutdown request or has closed socket

		lk.lock();
		// remove data from queue
		q.erase(q.begin(), q.begin() + count);
	}

	return true;
}

void ServerSocket::reset(unsigned maxevents) {
	ZoneScoped;
	printf("%s: maxevents %u\n", __func__, maxevents);
	events.resize(maxevents);
	peers.clear();
	peer_host = INVALID_SOCKET;

	running = true;
}

int ServerSocket::mainloop(uint16_t port, int backlog, int (*proper_packet)(const std::deque<uint8_t>&), bool (*process_packet)(const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed, void *arg), void *process_arg, unsigned maxevents) {
	ZoneScoped;
	reset(maxevents);
	this->proper_packet = proper_packet;
	this->process_packet = process_packet;
	this->process_arg = process_arg;

	s.bind(port);
	s.listen(backlog);

	// NOTE: on Windows, epoll(7) will call WSAStartup once
	if ((h = epoll_create1(0)) == INVALID_HANDLE_VALUE)
		return 1;

	if (add_fd(s.s))
		return 1;

	s.set_nonblocking();

	for (int nfds; (nfds = epoll_wait(h, events.data(), events.size(), -1)) >= 0;) {
		for (int i = 0; i < nfds; ++i) {
			epoll_event *ev = &events[i];

			if (ev->data.fd == s.s) {
				incoming();
			} else if (!io_step(i)) {
				// error or done: close fd
				SOCKET s = ev->data.sock;
				::closesocket(s);
				int r = peers.erase(s);
				assert(r == 1);

				// if socket was host: terminate server
				if (s == peer_host) {
					stop();
					return 0;
				}
			}
		}
	}

	stop();
	return 1;
}

}
