#pragma once

#include <cstdint>

#include <stdexcept>

#include <atomic>
#include <deque>
#include <vector>
#include <map>
#include <mutex>

#include <condition_variable>
#include <utility>

#if _WIN32
// use lowercase header names as we may be cross compiling on a filesystem where filenames are case-sensitive (which is typical on e.g. linux)
#include <winsock2.h>
#endif

#include <wepoll.h>

// wepoll does not support EPOLLET
#ifndef EPOLLET
#define EPOLLET 0
#endif

#include "ctpl_stl.hpp"

namespace aoe {

class Net final {
public:
	Net();
	~Net();
};

class ServerSocket;

void set_nonblocking(SOCKET s, bool nonbl=true);

class TcpSocket final {
	std::atomic<int> s;
	friend ServerSocket;
public:
	TcpSocket();
	TcpSocket(SOCKET s) : s((int)s) {}
	TcpSocket(const TcpSocket &) = delete;
	TcpSocket(TcpSocket &&s) noexcept : s(s.s.load()) { s.s.store((int)INVALID_SOCKET); }
	~TcpSocket();

	void open(); // manually create socket, closes old one
	void close();

	// server mode functions

	void bind(uint16_t port) { bind("0.0.0.0", port); }
	void bind(const char *address, uint16_t port);
	void listen(int backlog);
	SOCKET accept();
	SOCKET accept(sockaddr &a, int &sz);

	// client/peer mode functions

	void connect(const char *address, uint16_t port);

	// data exchange
	// NOTE tries indicates number of attempts. use tries=0 for infinite retries.
	// NOTE the template send/recv version use a different tries default value than the non-template versions. its value is chosen randomly.
	// NOTE the template send/recv version also throw if an incomplete object is sent/received. if you don't want this, use the generic send/recv functions.

	int try_send(const void *ptr, int len, unsigned tries) noexcept;
	int send(const void *ptr, int len, unsigned tries = 1);

	template<typename T> int send(const T *ptr, int len, unsigned tries = 5) {
		int out = send((const void *)ptr, len * sizeof * ptr, tries);
		if (out % sizeof * ptr)
			throw std::runtime_error("wsa: send failed: incomplete object sent");
		return out / sizeof * ptr;
	}

	void send_fully(const void *ptr, int len);

	template<typename T> void send_fully(const T *ptr, int len) {
		send_fully((void *)ptr, len * sizeof * ptr);
	}

	int try_recv(void *dst, int len, unsigned tries) noexcept;
	int recv(void *dst, int len, unsigned tries = 1);

	template<typename T> int recv(T *ptr, int len, unsigned tries = 5) {
		int in = recv((void *)ptr, len * sizeof * ptr, tries);
		if (in % sizeof * ptr)
			throw std::runtime_error("wsa: recv failed: incomplete object received");
		return in / sizeof * ptr;
	}

	void recv_fully(void *dst, int len);

	template<typename T> void recv_fully(T *ptr, int len) {
		recv_fully((void *)ptr, len * sizeof * ptr);
	}

	/** Change non-blocking mode. If true, recv_fully and send_fully become undefined! */
	void set_nonblocking(bool nonbl=true) {
		aoe::set_nonblocking(s, nonbl);
	}

	// operator overloading

	TcpSocket &operator=(TcpSocket &&other) noexcept
	{
		s.store(other.s.load());
		other.s.store((int)INVALID_SOCKET);
		return *this;
	}
};

class Peer final {
public:
	const std::string host, server;

	Peer(const char *host, const char *server) : host(host), server(server) {}

	friend bool operator<(const Peer &lhs, const Peer &rhs) {
		return std::tie(lhs.host, lhs.server) < std::tie(rhs.host, rhs.server);
	}
};

// TODO make multi thread-safe: mutex for open, stop, close, parts of mainloop
class ServerSocket final {
	TcpSocket s;
	HANDLE h;
	uint16_t port;
	std::vector<epoll_event> events;
	std::map<SOCKET, Peer> peers;
	SOCKET peer_host;
	std::mutex peer_ev_lock, data_lock;
	std::map<SOCKET, std::deque<uint8_t>> data_in, data_out;
	std::atomic_bool running;
	int (*proper_packet)(const std::deque<uint8_t>&);
	bool (*process_packet)(const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed, void *arg);
	void *process_arg;
	bool step;
public:
	ServerSocket();
	~ServerSocket();

	void open(const char *addr, uint16_t port, unsigned backlog=1);
	SOCKET accept() { return s.accept(); }
	SOCKET accept(sockaddr &a, int &sz) { return s.accept(a, sz); }

	void stop();
	void close();

	/**
	 * Start event loop to accept host and peers and manage all incoming network I/O.
	 * NOTE: on Windows, this will call WSAStartup (just once) when the epoll library starts up.
	 *
	 * arguments:
	 * int (*proper_packet)(const std::deque<uint8_t>&):
	 *   determines whether the packet received so far is complete
	 *   returns 0 if it needs more data
	 *   returns a positive number to indicate at least one packet has been received.
	 *   returns a negative number to drop the first X bytes in the queue. E.g.: -3 indicates 3 bytes have to be removed
	 * 
	 * bool (*process_packet)(std::deque<uint8_t> &in, std::deque<uint8_t> &out, int arg):
	 *   process received packet that's considered valid according to proper_packet reading from in and sending any response to out
	 *   the received data is in in, the data to be sent can be put in out.
	 *   arg is the value that was returned by proper_packet. arg will always be positive.
	 *   return false if you want to drop the connected client. true to keep it open.
	 */
	int mainloop(uint16_t port, int backlog, int (*proper_packet)(const std::deque<uint8_t>&), bool (*process_packet)(const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed, void *arg), void *process_arg, unsigned maxevents=256);
private:
	void reset(unsigned maxevents);

	int add_fd(SOCKET s);
	int del_fd(SOCKET s);

	void incoming();
	bool io_step(int idx);

	bool recv_step(const Peer &p, SOCKET s);
	bool send_step(SOCKET s);

	void remove_peer(SOCKET s);
};

}
