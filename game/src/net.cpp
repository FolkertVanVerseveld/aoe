#include "net.hpp"

#include <stdexcept>
#include <string>

#if _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#endif

namespace aoe {

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

}
