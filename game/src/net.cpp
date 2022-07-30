#include "net.hpp"

#include <cstdio>

#include <string>

#if _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#endif

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

SOCKET TcpSocket::accept() {
	return ::accept(s, NULL, NULL);
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
	int in = recv(ptr, len, 0);
	if (in != len) {
		if (in < 0)
			in = 0;
		throw std::runtime_error(std::string("tcp: recv_fully failed: ") + std::to_string(in) + (in == 1 ? " byte read out of " : " bytes read out of ") + std::to_string(len));
	}
}

}
