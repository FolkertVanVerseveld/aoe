#include "net.hpp"

#include "../os_macros.hpp"

#include <cassert>
#include <cstring>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>
#include <type_traits>

#if linux
#include <arpa/inet.h>
#endif

#include "../endian.h"

namespace genie {

extern void sock_block(sockfd fd, bool enabled);

static inline void dump(const void *buf, unsigned len) {
	for (unsigned i = 0; i < len; ++i)
		printf(" %02hX", *((const unsigned char*)buf + i));
}

const unsigned cmd_sizes[] = {
	TEXT_LIMIT,
};

void CmdData::hton() {}
void CmdData::ntoh() {}

void Command::hton() {
	type = htobe16(type);
	length = htobe16(length);
	data.hton();
}

void Command::ntoh() {
	type = be16toh(type);
	length = be16toh(length);
	data.ntoh();
}

std::string Command::text() {
	assert(type == CmdType::TEXT);
	data.text[TEXT_LIMIT - 1] = '\0';
	return data.text;
}

Command Command::text(const std::string &str) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = CmdType::TEXT];
	strncpy(cmd.data.text, str.c_str(), cmd.length);

	return cmd;
}

Socket::Socket(uint16_t port) : Socket() {
	this->port = port;
}

void Socket::reuse(bool enabled) {
	int val = enabled;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&val, sizeof val))
		throw std::runtime_error(std::string("Could not reuse TCP Socket: code ") + std::to_string(net_get_error()));
}

void Socket::bind() {
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(port);

	if (::bind(fd, (struct sockaddr*)&sa, sizeof sa))
		throw std::runtime_error(std::string("Could not bind TCP socket: code ") + std::to_string(net_get_error()));
}

int Socket::connect() {
	return connect(INADDR_LOOPBACK);
}

int Socket::connect(uint32_t addr, bool netorder) {
	struct sockaddr_in sa;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = netorder ? addr : htonl(addr);
	sa.sin_port = htons(port);

	return ::connect(fd, (struct sockaddr*)&sa, sizeof sa);
}

void Socket::listen() {
	if (::listen(fd, SOMAXCONN))
		throw std::runtime_error(std::string("Could not listen with TCP socket: code ") + std::to_string(net_get_error()));
}

void Socket::block(bool enabled) {
	sock_block(fd, enabled);
}

int Socket::send(const void *buf, unsigned len) {
	return ::send(fd, (const char*)buf, len, 0);
}

int Socket::recv(void *buf, unsigned len) {
	return ::recv(fd, (char*)buf, len, 0);
}

void Socket::sendFully(const void *buf, unsigned len) {
	for (unsigned sent = 0, rem = len; rem;) {
		int out;

		if ((out = send((const char*)buf + sent, rem)) <= 0)
			throw std::runtime_error(std::string("Could not send TCP data: code ") + std::to_string(net_get_error()));

		dump((const char*)buf + sent, rem);

		sent += out;
		rem -= out;
	}

	putchar('\n');
}

void Socket::recvFully(void *buf, unsigned len) {
	for (unsigned got = 0, rem = len; rem;) {
		int in;

		if ((in = recv((char*)buf + got, rem)) <= 0)
			throw std::runtime_error(std::string("Could not receive TCP data: code ") + std::to_string(net_get_error()));

		got += in;
		rem -= in;
	}
}

int Socket::recv(Command &cmd) {
	try {
		recvFully((void*)&cmd, CMD_HDRSZ);

		uint16_t type = be16toh(cmd.type), length = be16toh(cmd.length);

		if (type >= CmdType::MAX || length != cmd_sizes[type])
			return 2;

		recvFully((char*)&cmd + CMD_HDRSZ, length);
		cmd.ntoh();
		return 0;
	} catch (const std::runtime_error&) {
		return 1;
	}
}

void Socket::send(Command &cmd, bool net_order) {
	if (!net_order)
		cmd.hton();
	sendFully((const void*)&cmd, CMD_HDRSZ + cmd_sizes[be16toh(cmd.type)]);
}

ServerSocket::~ServerSocket() {
	close();
}

CmdBuf::CmdBuf(sockfd fd) : size(0), transmitted(0), endpoint(fd) {}

CmdBuf::CmdBuf(sockfd fd, const Command &cmd, bool net_order) : size(0), transmitted(0), endpoint(fd), cmd(cmd) {
	if (!net_order)
		this->cmd.hton();

	size = CMD_HDRSZ + be16toh(this->cmd.length);
}

int CmdBuf::read(ServerCallback &cb, char *buf, unsigned len) {
	unsigned processed = 0;

	printf("read: buf=%p, len=%u\n", (void*)buf, len);
	while (len) {
		unsigned need;

		// wait for complete header before processing any data
		if (transmitted < CMD_HDRSZ) {
			puts("waiting for header data");
			need = CMD_HDRSZ - transmitted;

			if (need > len) {
				memcpy((char*)&cmd + transmitted, buf + processed, len);
				transmitted += len;
				processed += len;
				return 0; // stop, we need more data
			}

			puts("got header");

			memcpy((char*)&cmd + transmitted, buf + processed, need);
			transmitted += need;
			processed += need;
			len -= need;
		}

		// validate header
		unsigned type = be16toh(cmd.type), length = be16toh(cmd.length);

		if (type >= CmdType::MAX || length != cmd_sizes[type]) {
			if (type < CmdType::MAX)
				fprintf(stderr, "bad header: type %u, size %u (expected %u)\n", type, length, cmd_sizes[type]);
			else
				fprintf(stderr, "bad header: type %u, size %u\n", type, length);
			return 1;
		}

		assert(transmitted >= CMD_HDRSZ);
		size = CMD_HDRSZ + length;

		if (transmitted < size) {
			// packet incomplete, read only as much as we need
			need = size - transmitted;
			printf("need %u more bytes (%u available)\n", need, len);

			unsigned copy = need < len ? need : len;

			memcpy(((char*)&cmd) + transmitted, buf + processed, copy);
			transmitted += copy;
			len -= copy;
			processed += copy;
			printf("read %u bytes: trans, len: %u, %u\n", copy, transmitted, len);
		}

		// only process full packets
		if (transmitted >= size) {
			cmd.ntoh();

			printf("process: %d, %d\n", transmitted, size);

			dump((char*)&cmd, transmitted);
			putchar('\n');

			cb.event_process(endpoint, cmd);

			// move pending data to front
			memmove((char*)&cmd, ((char*)&cmd) + size, transmitted - size);

			// update state
			transmitted -= size;
			size = 0;

			printf("trans, size: %d, %d\n", transmitted, size);
		}
	}

	return 0;
}

SSErr CmdBuf::write() {
	if (transmitted == size)
		return SSErr::OK;

	int n = ::send(endpoint, (char*)&cmd + transmitted, size - transmitted, 0);
	if (n <= 0)
		return SSErr::WRITE;

	transmitted += (unsigned)n;
	return transmitted == size ? SSErr::OK : SSErr::PENDING;
}

void ServerSocket::broadcast(Command &cmd, bool net_order) {
	if (!net_order)
		cmd.hton();

	printf("broadcast to %u peers\n", (unsigned)peers.size());

	for (auto &x : peers)
		if (push(pollfd(x), cmd, true) != SSErr::OK)
			throw std::runtime_error(std::string("broadcast failed for fd ") + std::to_string(pollfd(x)));
}

bool operator<(const CmdBuf &lhs, const CmdBuf &rhs) {
	return lhs.endpoint < rhs.endpoint;
}

}
