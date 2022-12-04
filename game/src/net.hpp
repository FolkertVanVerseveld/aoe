#pragma once

#include <cstdint>

#include <stdexcept>

#include <atomic>
#include <deque>
#include <vector>
#include <map>
#include <mutex>
#include <thread>

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

#include "debug.hpp"
#include "ctpl_stl.hpp"

namespace aoe {

static constexpr unsigned tcp4_max_size = UINT16_MAX - 32 + 1;

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
	int send(const void *ptr, int len, unsigned tries=1);

	template<typename T> int send(const T *ptr, int len, unsigned tries=5) {
		int out = send((const void *)ptr, len * sizeof * ptr, tries);
		if (out % sizeof * ptr)
			throw std::runtime_error("wsa: send failed: incomplete object sent");
		return out / sizeof * ptr;
	}

	void send_fully(const void *ptr, int len);

	template<typename T> void send_fully(const T *ptr, int len) {
		send_fully((void*)ptr, len * sizeof * ptr);
	}

	int try_recv(void *dst, int len, unsigned tries) noexcept;
	int recv(void *dst, int len, unsigned tries=1);

	template<typename T> int recv(T *ptr, int len, unsigned tries=5) {
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
	const SOCKET sock;
	const std::string host, server;
	const bool is_host;

	Peer(SOCKET sock, const char *host, const char *server, bool is_host) : sock(sock), host(host), server(server), is_host(is_host) {}

	friend bool operator<(const Peer &lhs, const Peer &rhs) {
		return std::tie(lhs.host, lhs.server) < std::tie(rhs.host, rhs.server);
	}
};

class ServerSocketController {
public:
	ServerSocketController() {}
	virtual ~ServerSocketController() {}

	/* A peer has just connected to this server. */
	virtual void incoming(ServerSocket &s, const Peer &p) = 0;

	/* A peer has just left or has been kicked from this server. */
	virtual void dropped(ServerSocket &s, const Peer &p) = 0;

	/* Server has been stopped. */
	virtual void stopped() = 0;

	/*
	 * determines whether the packet received so far is complete
	 *   returns 0 if it needs more data
	 *   returns a positive number to indicate at least one packet has been received.
	 *   returns a negative number to drop the first X bytes in the queue. E.g.: -3 indicates 3 bytes have to be removed
	 */
	virtual int proper_packet(ServerSocket &s, const std::deque<uint8_t>&) = 0;

	/*
	 * process received packet that's considered valid according to proper_packet reading from in and sending any response to out
	 *   the received data is in in, the data to be sent can be put in out.
	 *   processed is the value that was returned by proper_packet. processed will always be positive.
	 *   return false if you want to drop the connected client. true to keep it open.
	 */
	virtual bool process_packet(ServerSocket &s, const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed) = 0;
};

// TODO make multi thread-safe: mutex for open, stop, close, parts of mainloop
class ServerSocket final {
	TcpSocket s;
	HANDLE h;
	uint16_t port;
	std::vector<epoll_event> events;
	std::map<SOCKET, Peer> peers;
	SOCKET peer_host;
	std::mutex peer_ev_lock, data_lock, m_pending;
	std::map<SOCKET, std::deque<uint8_t>> data_in, data_out;
	std::vector<char> recvbuf, sendbuf;
	std::atomic<bool> running;
	bool step;
	std::atomic<unsigned long long> poll_us;
	std::vector<SOCKET> closing;
	std::map<SOCKET, std::deque<uint8_t>> send_pending;
	std::atomic<std::thread::id> id;

	std::mutex m_ctl;
	ServerSocketController *ctl;
	friend Debug;
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
	 * 
	 * Keep in mind that proper_packet and process_packet are called directly from this mainloop. This means that any pending incoming network data processing will be halted until the callbacks are completed. For best responsiveness, forward the data to another thread to process it.
	 */
	int mainloop(uint16_t port, int backlog, ServerSocketController &ctl, unsigned recvbuf=512, unsigned sendbuf=1024);

	/**
	 * Change poll timeout (0 to disable). Note that this is only used on systems
	 * that don't support edge triggered events for epoll(7), like Windows.
	 * Defaults to 50,000us (50 milliseconds).
	 */
	void set_poll_timeout(unsigned long long microseconds) { poll_us = microseconds; }

	void send(const Peer &p, const void *ptr, int len);
	void broadcast(const void *ptr, int len, bool include_host=true);
private:
	void reset(ServerSocketController &ctl, unsigned recvbuf, unsigned sendbuf);

	int add_fd(SOCKET s);
	int del_fd(SOCKET s);

	void incoming();
	bool io_step(int idx);

	bool recv_step(const Peer &p, SOCKET s);
	bool send_step(SOCKET s);

	bool event_step(int idx);
	void reduce_peers();
	void flush_queue();

	void queue_out(const Peer &p, const void *ptr, int len);
};

}
