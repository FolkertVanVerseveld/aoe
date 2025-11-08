#include <cstdio>

#if _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

#include "src/net/net.hpp"

using namespace aoe;

static int reuse_socket(SOCKET s, BOOL v=true) {
	int err = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&v, sizeof v);
	return err != SOCKET_ERROR ? 0 : WSAGetLastError();
}

static void incoming(SOCKET listenSock, std::vector<WSAPOLLFD> &fds)
{
	for (;;) {
		sockaddr_in cli{0};
		int err, clen = sizeof cli;
		SOCKET cs;

		if ((cs = accept(listenSock, (sockaddr *)&cli, &clen)) == INVALID_SOCKET) {
			if ((err = WSAGetLastError()) != WSAEWOULDBLOCK)
				fprintf(stderr, "%s: accept failed with code 0x%X\n", __func__, err);

			break;
		}

		set_nonblocking(cs);

		unsigned long long fd = (unsigned long long)cs;
		char ip[INET_ADDRSTRLEN]{};

		if (!inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof ip)) {
			fprintf(stderr, "%s: unknown ip. closing %llu\n", __func__, fd);
			closesocket(cs);
		} else {
			WSAPOLLFD cfd{0};
			cfd.fd = cs;
			cfd.events = POLLRDNORM | POLLERR | POLLHUP;
			cfd.revents = 0;
			fds.push_back(cfd);

			printf("Accepted %s:%d (socket=%llu)\n", ip, ntohs(cli.sin_port), fd);
		}
	}
}

static bool serve_client(WSAPOLLFD &p)
{
	if (p.revents & (POLLERR | POLLHUP | POLLNVAL))
		return false;

	if (!(p.revents & POLLRDNORM))
		// no new data
		return true;

	constexpr int BUFSZ = 4096;
	char buf[BUFSZ];

	for (;;) {
		int err, n = ::recv(p.fd, buf, BUFSZ, 0);
		if (n < 0) {
			if ((err = WSAGetLastError()) == WSAEWOULDBLOCK)
				// out of data
				return true;

			fprintf(stderr, "%s: error %d: closing\n", __func__, err);
			return false;
		} else if (n == 0) {
			// peer has closed connection
			return false;
		}

		// TODO echo back
	}

	return true;
}

int main(int argc, char **argv) {
	Net net;
	long port = 27027; // https://xkcd.com/221/
	const int backlog = 10; // SOMAXCONN is overkill...

	if (argc > 1) {
		char *arg = argv[1], *end = NULL;
		port = strtol(arg, &end, 10);
		if (!port && end == arg) {
			fprintf(stderr, "invalid port: \"%s\" should be integer and in range (0, 65535]\n", arg);
			return 1;
		}
	}

	if (port <= 0 || port > 65535) {
		fprintf(stderr, "invalid port: out of range (0, 65535], got %ld\n", port);
		return 1;
	}

	SOCKET listenSock;
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	int err;

	// TODO refactor to TcpSocket
	if ((listenSock = socket(addr.sin_family, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		fprintf(stderr, "socket: failed to create listenSock: code 0x%X\n", WSAGetLastError());
		return 1;
	}

	if (!!(err = reuse_socket(listenSock)))
		fprintf(stderr, "reuse_socket: failed with code 0x%X. ignoring...\n", err);

	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((u_short)port);

	if (bind(listenSock, (const sockaddr*)&addr, sizeof addr) == SOCKET_ERROR) {
		fprintf(stderr, "bind: failed with code 0x%X\n", WSAGetLastError());
		return 1;
	}

	if (listen(listenSock, backlog) == SOCKET_ERROR) {
		fprintf(stderr, "listen: failed with code 0x%X\n", WSAGetLastError());
		return 1;
	}

	set_nonblocking(listenSock);

	std::vector<WSAPOLLFD> fds;
	fds.reserve(64);

	WSAPOLLFD lfd{};
	lfd.fd = listenSock;
	lfd.events = POLLRDNORM;
	lfd.revents = 0;
	fds.push_back(lfd);

	for (int rc; (rc = WSAPoll(fds.data(), (ULONG)fds.size(), -1)) != SOCKET_ERROR;) {
		for (size_t i = 1; i < fds.size();) {
			if (serve_client(fds[i])) {
				fds[i++].revents = 0;
				continue;
			}

			printf("closing client socket %llu\n", (unsigned long long)fds[i].fd);
			closesocket(fds[i].fd);
			fds[i] = fds.back();
			fds.pop_back();
		}

		if (fds[0].revents & POLLRDNORM)
			incoming(listenSock, fds);

		fds[0].revents = 0;
	}

	switch (WSAGetLastError()) {
	case WSAENETDOWN:
		fputs("WSAPoll: failed because network system has shutdown\n", stderr);
		break;
	case WSAEFAULT:
		fputs("WSAPoll: failed because exception occurred while reading user input parameters\n", stderr);
		break;
	case WSAEINVAL:
		fputs("WSAPoll: failed because an invalid parameter was passed\n", stderr);
		break;
	case WSAENOBUFS:
		fputs("WSAPoll: failed because out of socket resources\n", stderr);
		break;
	default:
		fprintf(stderr, "WSAPoll: failed with code 0x%X\n", WSAGetLastError());
		break;
	}

	return 0;
}
#else
int main(int argc, char **argv) {
	fputs("dedicated server only supported on Windows...\n", stderr);
	return 1;
}
#endif
