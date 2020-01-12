#pragma once

#include "os_macros.hpp"

#include <cstdint>

#if windows
typedef uintptr_t sockfd;
#else
typedef int sockfd;
#endif

class Net final {
public:
	Net();
	~Net();
};

class TcpSocket final {
	sockfd fd;
public:
	TcpSocket();
	~TcpSocket();
};
