#include "net.hpp"

#include "os_macros.hpp"

#include <cassert>

#include <stdexcept>
#include <string>
#include <type_traits>

#if windows
static_assert(std::is_same<SOCKET, sockfd>::value, "SOCKET must match sockfd type");

	#define WIN32_LEAN_AND_MEAN

	#include <Windows.h>
	#include <WinSock2.h>

#define WSA_VERSION_MINOR 2
#define WSA_VERSION_MAJOR 2

#pragma comment(lib, "ws2_32.lib")

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

TcpSocket::TcpSocket() {
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
		throw std::runtime_error(std::string("Could not create TCP socket: code ") + std::to_string(WSAGetLastError()));
}

TcpSocket::~TcpSocket() {
	closesocket(fd);
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

Net::Net() {}
Net::~Net() {}

TcpSocket::TcpSocket() {
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		throw std::runtime_error(std::string("Could not create TCP socket: ") + strerror(errno));
}

TcpSocket::~TcpSocket() {
	close(fd);
}
#else
#error stub
#endif
