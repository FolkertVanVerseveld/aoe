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

	unsigned v = ++initnet;
	(void)v;// printf("%s: initnet=%u\n", __func__, v);
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

	unsigned v = --initnet;
	(void)v;// printf("%s: initnet=%u\n", __func__, v);
}

TcpSocket::TcpSocket() : s((int)INVALID_SOCKET) {
	SOCKET sock;
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		throw std::runtime_error("socket failed");

	s.store((int)sock);
}

TcpSocket::~TcpSocket() {
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-closesocket
	closesocket(s);
}

void TcpSocket::open() {
	SOCKET sock;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		throw std::runtime_error("socket failed");

	closesocket(s);
	s.store((int)sock);
}

void TcpSocket::close() {
	if (!closesocket((int)s))
		s = INVALID_SOCKET;
}

SOCKET TcpSocket::accept() {
	return ::accept(s, NULL, NULL);
}

SOCKET TcpSocket::accept(sockaddr &a, int &sz) {
	return ::accept(s, &a, &sz);
}

void TcpSocket::bind(const char *address, uint16_t port) {
	const auto sock = s.load(std::memory_order_relaxed);
	sockaddr_in dst{ 0 };

	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = inet_addr(address);
	dst.sin_port = htons(port);

	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
	int r = ::bind(sock, (const sockaddr *)&dst, sizeof dst);

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

	const auto sock = s.load(std::memory_order_relaxed);
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	int r = ::listen(sock, backlog);

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
	const auto sock = s.load(std::memory_order_relaxed);
	sockaddr_in dst{ 0 };

	dst.sin_family = AF_INET;
	dst.sin_addr.s_addr = inet_addr(address);
	dst.sin_port = htons(port);

	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect
	int r = ::connect(sock, (const sockaddr *)&dst, sizeof dst);

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
	const auto sock = s.load(std::memory_order_relaxed);
	int written = 0;

	while (written < len) {
		int rem = len - written;
		int out = ::send(sock, (const char *)ptr + written, rem, 0);

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

	int err = WSAGetLastError();

	switch (err) {
		case 0:
			throw std::runtime_error(std::string("wsa: send failed: unknown return code ") + std::to_string(out));
		default:
			wsa_generic_error("wsa: send failed", err);
			break;
	}

	return out;
}

void TcpSocket::send_fully(const void *ptr, int len) {
	int out;
	
	if ((out = send(ptr, len, 0)) == len)
		return;

	if (!out)
		throw SocketClosedError("tcp: send_fully failed: connection closed");

	if (out < 0)
		out = 0;

	throw std::runtime_error(std::string("tcp: send_fully failed: ") + std::to_string(out) + (out == 1 ? " byte written out of " : " bytes written out of ") + std::to_string(len));
}

int TcpSocket::try_recv(void *dst, int len, unsigned tries) noexcept {
	const auto sock = s.load(std::memory_order_relaxed);
	int read = 0;

	while (read < len) {
		int rem = len - read;
		int in = ::recv(sock, (char *)dst + read, rem, 0);

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

	int err = WSAGetLastError();

	if (!err && in < 0)
		throw std::runtime_error(std::string("wsa: recv failed: unknown return code ") + std::to_string(in));

	wsa_generic_error("wsa: recv failed", err);

	return in;
}

void TcpSocket::recv_fully(void *ptr, int len) {
	int in;

	if ((in = recv(ptr, len, 0)) == len)
		return;

	if (!in)
		throw SocketClosedError("tcp: recv_fully failed: connection closed");

	if (in < 0)
		in = 0;

	throw std::runtime_error(std::string("tcp: recv_fully failed: ") + std::to_string(in) + (in == 1 ? " byte read out of " : " bytes read out of ") + std::to_string(len));
}

ServerSocket::ServerSocket() : s(), h(INVALID_HANDLE_VALUE), port(0), events(), peers(), peer_host(INVALID_SOCKET), peer_ev_lock(), data_lock(), m_pending(), data_in(), data_out(), recvbuf(), sendbuf(), running(false), step(false), poll_us(50u * 1000ull), closing(), send_pending(), id(std::this_thread::get_id()), m_ctl(), ctl(nullptr) {}

ServerSocket::~ServerSocket() { stop(); }

void ServerSocket::open(const char *addr, uint16_t port, unsigned backlog) {
	ZoneScoped;
	s.bind(addr, port);
	s.listen(backlog);
}

void ServerSocket::stop() {
	ZoneScoped;

	std::unique_lock<std::mutex> lk(peer_ev_lock);

	for (const epoll_event &ev : events) {
		SOCKET s = ev.data.sock;
		del_fd(s);
		::closesocket(s);
	}

	events.clear();
	peers.clear();

	if (h != INVALID_HANDLE_VALUE)
		if (!epoll_close(h))
			h = INVALID_HANDLE_VALUE;

	lk.unlock();

	running = false;

	std::lock_guard<std::mutex> lk2(m_ctl);
	if (ctl)
		ctl->stopped();

	ctl = nullptr;
}

void ServerSocket::close() {
	ZoneScoped;
	stop();
	s.close();
}

int ServerSocket::add_fd(SOCKET s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lk(peer_ev_lock);
	epoll_event ev{ 0 };

	events.emplace_back(ev);

	ev.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
	ev.data.fd = (int)s;

	return epoll_ctl(h, EPOLL_CTL_ADD, s, &ev);
}

int ServerSocket::del_fd(SOCKET s) {
	ZoneScoped;
	epoll_event ev{ 0 };
	ev.data.fd = (int)s;

	return epoll_ctl(h, EPOLL_CTL_DEL, s, &ev);
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

		hbuf[NI_MAXHOST - 1] = sbuf[NI_MAXSERV - 1] = '\0';

		// set up peer
		printf("%s: descriptor %u: %s:%s\n", __func__, (unsigned)infd, hbuf, sbuf);
		set_nonblocking(infd);
		if (add_fd(infd))
			throw std::runtime_error("ssock: add_fd failed");

		std::string host(hbuf);
		bool is_host = false;

		// check if peer is host
		if (peer_host == INVALID_SOCKET && host == "127.0.0.1") {
			printf("%s: host joined at service %s\n", __func__, sbuf);
			peer_host = infd;
			is_host = true;
		}

		auto ins = peers.emplace(std::piecewise_construct, std::forward_as_tuple(infd), std::forward_as_tuple(infd, hbuf, sbuf, is_host));
		assert(ins.second);

		// now just let the controller know a new client has joined
		bool keep = false;
		{
			std::lock_guard<std::mutex> lk(m_ctl);

			if (ctl)
				keep = ctl->incoming(*this, ins.first->second);
		}

		// roll back if controller decided we need to drop the client
		if (!keep) {
			del_fd(infd);
			peers.erase(infd);

			if (is_host)
				peer_host = INVALID_SOCKET;
		}
	}
}

bool ServerSocket::io_step(int idx) {
	ZoneScoped;

	std::unique_lock<std::mutex> lk(peer_ev_lock);

	epoll_event &ev = events.at(idx);
	SOCKET s = ev.data.sock;

	auto it = peers.find(s);
	if (it == peers.end())
		return false;

	bool r = recv_step(it->second, s);

	lk.unlock();

	return r && send_step(s);
}

bool ServerSocket::recv_step(const Peer &p, SOCKET s) {
	ZoneScoped;

	while (1) {
		int count = ::recv(s, recvbuf.data(), recvbuf.size(), 0);
		if (count < 0) {
			int r = WSAGetLastError();

			if (r != WSAEWOULDBLOCK) {
				fprintf(stderr, "%s: recv failed: code %d\n", __func__, r);
				return false;
			}

			return true;
		}

		if (count == 0) {
			fprintf(stderr, "%s: nothing received for %s:%s\n", __func__, p.host.c_str(), p.server.c_str());
			return false; // peer send shutdown request or has closed socket
		}

		step = true;
		std::unique_lock<std::mutex> lk(data_lock);

		auto ins = data_in.try_emplace(s);
		auto it = ins.first;
		for (int i = 0; i < count; ++i)
			it->second.emplace_back(recvbuf[i]);

		std::unique_lock<std::mutex> lkctl(m_ctl);
		int processed = 0;

		while ((processed = ctl->proper_packet(*this, it->second)) > 0) {
			bool keep_alive = false;

			try {
				auto outs = data_out.try_emplace(s);
				auto out = outs.first;

				keep_alive = ctl->process_packet(*this, p, it->second, out->second, processed);
			} catch (const std::runtime_error &e) {
				fprintf(stderr, "%s: failed to process for (%s,%s): %s\n", __func__, p.host.c_str(), p.server.c_str(), e.what());
			}

			if (!keep_alive)
				return false;
		}

		lkctl.unlock();

		// remove bytes if asked to do so
		for (; processed < 0 && !it->second.empty(); ++processed)
			it->second.pop_front();

		lk.unlock();
	}
}

bool ServerSocket::send_step(SOCKET s) {
	ZoneScoped;
	std::unique_lock<std::mutex> lk(data_lock);

	auto it = data_out.find(s);
	if (it == data_out.end())
		return true;

	auto &q = it->second;
	while (!q.empty()) {
		printf("%s: try send %llu bytes to %d\n", __func__, (unsigned long long)q.size(), (int)s);
		int count;
		int out = (int)std::min(sendbuf.size(), q.size());

		for (int i = 0; i < out; ++i)
			sendbuf[i] = q[i];

		// s may be closed after this unlock, but this way, we give a brief moment for other threads to kick in
		lk.unlock();

		count = ::send(s, sendbuf.data(), out, 0);
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

		step = true;
		lk.lock();
		// remove data from queue
		q.erase(q.begin(), q.begin() + count);
	}

	return true;
}

void ServerSocket::reset(ServerSocketController &ctl, unsigned recvbuf, unsigned sendbuf) {
	ZoneScoped;

	if (recvbuf < 1)
		throw std::runtime_error("recvbuf must be positive");

	if (sendbuf < 1)
		throw std::runtime_error("sendbuf must be positive");

	std::unique_lock<std::mutex> lk(peer_ev_lock, std::defer_lock), lk2(data_lock, std::defer_lock);
	std::lock(lk, lk2);

	assert(!running);
	events.clear();
	peers.clear();
	peer_host = INVALID_SOCKET;

	data_in.clear();
	data_out.clear();

	this->recvbuf.resize(recvbuf);
	this->sendbuf.resize(sendbuf);

	closing.clear();
	id = std::this_thread::get_id();

	std::lock_guard<std::mutex> lks(m_pending);
	send_pending.clear();

	std::lock_guard<std::mutex> lkctl(m_ctl);
	this->ctl = &ctl;

	running = true;
}

void ServerSocket::reduce_peers() {
	ZoneScoped;

	if (closing.empty())
		return;

	std::unique_lock<std::mutex> plk(peer_ev_lock), dlk(data_lock), slk(m_pending);

	for (SOCKET sock : closing) {
		del_fd(sock);
		::closesocket(sock);
		data_out.erase(sock);
		send_pending.erase(sock);
	}

	slk.unlock();
	dlk.unlock();

	std::unique_lock<std::mutex> mctl(m_ctl);

	for (SOCKET sock : closing) {
		if (ctl)
			ctl->dropped(*this, peers.at(sock));

		peers.erase(sock);
	}

	closing.clear();
}

bool ServerSocket::event_step(int idx) {
	ZoneScoped;

	epoll_event *ev = &events[idx];
	//printf("events = %08X\n", ev->events);

	if (ev->data.fd == s.s) {
		step = true;
		incoming();
	} else if (!io_step(idx)) {
		// error or done: close fd
		SOCKET s = ev->data.sock;

		// if socket was host: terminate server
		if (s == peer_host)
			return false;

		// postpone closing socket to the end
		closing.emplace_back(s);
	}

	return true;
}

void ServerSocket::queue_out(const Peer &p, const void *ptr, int len) {
	SOCKET sock = p.sock;

	printf("%s: %d bytes for %s:%s\n", __func__, len, p.host.c_str(), p.server.c_str());

	auto it = send_pending.find(sock);
	if (it == send_pending.end())
		it = send_pending.try_emplace(sock).first;

	std::deque<uint8_t> &out = it->second;

	const uint8_t *src = (const uint8_t*)ptr;

	for (int i = 0; i < len; ++i)
		out.emplace_back(src[i]);
}

void ServerSocket::send(const Peer &p, const void *ptr, int len) {
	std::lock_guard<std::mutex> lk(m_pending);
	queue_out(p, ptr, len);
}

void ServerSocket::broadcast(const void *ptr, int len, bool include_host) {
	const auto id = this->id.load(std::memory_order_relaxed);

	std::unique_lock<std::mutex> lk(m_pending, std::defer_lock);
	std::unique_lock<std::mutex> plk(peer_ev_lock, std::defer_lock);

	// only lock if not ran from mainloop thread
	if (std::this_thread::get_id() != id)
		std::lock(lk, plk);
	else
		lk.lock();

	for (auto kv : peers) {
		const auto p = kv.second;

		if (!include_host && peer_host == p.sock)
			continue;

		queue_out(p, ptr, len);
	}
}

void ServerSocket::flush_queue() {
	std::lock_guard<std::mutex> lk(m_pending);

	if (send_pending.empty())
		return;

	for (auto it = send_pending.begin(); it != send_pending.end();) {
		auto kv = *it;
		SOCKET sock = kv.first;
		auto q = kv.second;

		// remove if nothing to send anymore
		if (q.empty()) {
			it = send_pending.erase(it);
			continue;
		}

		// try to send any pending data
		std::unique_lock<std::mutex> lk(data_lock);

		auto it2 = data_out.find(sock);
		std::deque<uint8_t> *q2 = nullptr;

		if (it2 == data_out.end()) {
			// no data_out item: use queue from send_pending
			q2 = &data_out.try_emplace(sock).first->second;
		} else {
			// data_out item present
			q2 = &it2->second;

			// this step is really important: if the out queue from send_step is not empty, there is no way we can guarantee that the api layer has sent a full message. so we have to wait for that queue to become depleted first.
			if (!q2->empty()) {
				++it;
				continue;
			}
		}

		// prepare to send
		std::swap(*q2, q);
		it = send_pending.erase(it);

		lk.unlock();

		send_step(sock);
	}
}

int ServerSocket::mainloop(uint16_t port, int backlog, ServerSocketController &ctl, unsigned recvbuf, unsigned sendbuf) {
	ZoneScoped;
	reset(ctl, recvbuf, sendbuf);

	s.bind(port);
	s.listen(backlog);

	// NOTE: on Windows, epoll(7) will call WSAStartup once
	if ((h = epoll_create1(0)) == INVALID_HANDLE_VALUE)
		return 1;

	if (add_fd(s.s))
		return 1;

	s.set_nonblocking();
	step = false;

	for (int nfds; (nfds = epoll_wait(h, events.data(), events.size(), -1)) >= 0; step = false) {
		for (int i = 0; i < nfds; ++i)
			if (!event_step(i)) {
				stop();
				return 0;
			}

		reduce_peers();
		flush_queue();

		unsigned long long dt = poll_us.load(std::memory_order_relaxed);

#if _WIN32
		if (!step && dt) {
			//puts("polling");
			std::this_thread::sleep_for(std::chrono::microseconds(dt));
		}
#endif
	}

	stop();
	return 1;
}

}
