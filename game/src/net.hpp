#pragma once

#include <cstdint>

#include <stdexcept>

#include <utility>

#if _WIN32
// use lowercase header names as we may be cross compiling on a filesystem where filenames are case-sensitive (which is typical on e.g. linux)
#include <winsock2.h>
#endif

namespace aoe {

class Net final {
public:
	Net();
	~Net();
};

class TcpSocket final {
	SOCKET s;
public:
	TcpSocket();
	TcpSocket(SOCKET s) : s(s) {}
	TcpSocket(const TcpSocket &) = delete;
	TcpSocket(TcpSocket &&s) noexcept : s(std::exchange(s.s, INVALID_SOCKET)) {}
	~TcpSocket();

	// server mode functions

	void bind(const char *address, uint16_t port);
	void listen(int backlog);
	SOCKET accept();

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

	// operator overloading

	TcpSocket &operator=(TcpSocket &&other) noexcept
	{
		s = std::exchange(other.s, INVALID_SOCKET);
		return *this;
	}
};

}
