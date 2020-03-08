/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#include "net.hpp"

#include "../os_macros.hpp"

// change this if you need to debug cmdbuf::read
#define DEBUG 0
#include "dbg.h"

#include <cassert>
#include <cstring>

#include <inttypes.h>

#include <chrono>
#include <thread>
#include <stdexcept>
#include <string>
#include <type_traits>

// linux needs in_addr and sockaddr_in
#if linux
#include <arpa/inet.h>
#endif

#include "../endian.h"
#include "../string.hpp"

namespace genie {

#if windows
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

extern void sock_block(sockfd fd, bool enabled);

static inline void dump(const void *buf, unsigned len) {
	for (unsigned i = 0; i < len; ++i)
		printf(" %02hX", *((const unsigned char*)buf + i));
}

const unsigned cmd_sizes[] = {
	sizeof(TextMsg),
	sizeof(JoinUser),
	sizeof(user_id),
	sizeof(StartMatch),
	sizeof(Ready),
	sizeof(CreatePlayer),
	sizeof(AssignSlave),
};

void CmdData::hton(uint16_t type) {
	assert(cmd_sizes[(unsigned)CmdType::max - 1]);
	static_assert(sizeof(JoinUser) == sizeof(user_id) + NAME_LIMIT);
	static_assert(sizeof(user_id) == sizeof(uint16_t));

	switch ((CmdType)type) {
	case CmdType::text:
		text.from = htobe16(text.from);
		break;
	case CmdType::join:
		join.id = htobe16(join.id);
		break;
	case CmdType::leave:
		leave = htobe16(leave);
		break;
	case CmdType::start:
		start.map_w = htobe16(start.map_w);
		start.map_h = htobe16(start.map_h);
		start.seed = htobe32(start.seed);
		start.slave_count = htobe16(start.slave_count);
		break;
	case CmdType::ready:
		ready.slave_count = htobe16(ready.slave_count);
		break;
	case CmdType::create:
		create.id = htobe16(create.id);
		break;
	case CmdType::assign:
		assign.from = htobe16(assign.from);
		assign.to = htobe16(assign.to);
		break;
	}
}

void CmdData::ntoh(uint16_t type) {
	switch ((CmdType)type) {
	case CmdType::text:
		text.from = be16toh(text.from);
		break;
	case CmdType::join:
		join.id = be16toh(join.id);
		break;
	case CmdType::leave:
		leave = be16toh(leave);
		break;
	case CmdType::start:
		start.map_w = be16toh(start.map_w);
		start.map_h = be16toh(start.map_h);
		start.seed = be32toh(start.seed);
		start.slave_count = be16toh(start.slave_count);
		break;
	case CmdType::ready:
		ready.slave_count = be16toh(ready.slave_count);
		break;
	case CmdType::create:
		create.id = be16toh(create.id);
		break;
	case CmdType::assign:
		assign.from = be16toh(assign.from);
		assign.to = be16toh(assign.to);
		break;
	}
}

void Command::hton() {
	data.hton(type);
	type = htobe16(type);
	length = htobe16(length);
}

void Command::ntoh() {
	type = be16toh(type);
	length = be16toh(length);
	data.ntoh(type);
}

std::string TextMsg::str() const {
	return std::string(text, strlen(text, TEXT_LIMIT));
}

CreatePlayer::CreatePlayer(player_id id, const std::string &str) : id(id), name() {
	strncpy(name, str.c_str(), NAME_LIMIT);
}

std::string CreatePlayer::str() const {
	return std::string(name, strlen(name, NAME_LIMIT));
}

TextMsg Command::text() {
	assert((CmdType)type == CmdType::text);
	return data.text;
}

std::string JoinUser::nick() {
	name[NAME_LIMIT - 1] = '\0';
	return name;
}

JoinUser::JoinUser(user_id id, const std::string &str) : id(id) {
	strncpy(name, str.c_str(), NAME_LIMIT);
}

JoinUser Command::join() {
	assert(type == (uint16_t)CmdType::join);
	return data.join;
}

Ready Command::ready() {
	assert(type == (uint16_t)CmdType::ready);
	return data.ready;
}

Command Command::join(user_id id, const std::string &str) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::join];
	cmd.data.join.id = id;
	strncpy(cmd.data.join.name, str.c_str(), NAME_LIMIT);

	return cmd;
}

Command Command::leave(user_id id) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::leave];
	cmd.data.leave = id;

	return cmd;
}

Command Command::text(user_id id, const std::string &str) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::text];
	cmd.data.text.from = id;
	strncpy(cmd.data.text.text, str.c_str(), TEXT_LIMIT);

	return cmd;
}

Command Command::start(StartMatch &settings) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::start];
	cmd.data.start = settings;

	return cmd;
}

Command Command::ready(uint16_t slave_count, uint16_t prng_next) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::ready];
	cmd.data.ready.slave_count = slave_count;

	return cmd;
}

Command Command::create(player_id id, const std::string &str) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::create];
	cmd.data.create.id = id;
	strncpy(cmd.data.create.name, str.c_str(), NAME_LIMIT);
	
	return cmd;
}

Command Command::assign(user_id id, player_id pid) {
	Command cmd;

	cmd.length = cmd_sizes[cmd.type = (uint16_t)CmdType::assign];
	cmd.data.assign.from = id;
	cmd.data.assign.to = pid;

	return cmd;
}

const unsigned map_sizes[] = {
	48, // 0
	72, // 1
	96, // 2
	120, // 3
	120, // 4
	144, // 5
	144, // 6
	168, // 7
	168, // 8
};

void StartMatch::dump() const {
	printf("startmatch settings:\n");
	printf("scenario type: %" PRIu8 ", options: %" PRIx8 "\n", scenario_type, options);
	printf("size: %" PRIu16 "x%" PRIu16 "\n", map_w, map_h);
	printf("human players: %" PRIu16 ", difficulty: %" PRIu8 "\n", slave_count, difficulty);
	printf("seed: %" PRIu32 ", map type: %" PRIu8 ", starting age: %" PRIu8 ", victory: %" PRIu8 "\n", seed, map_type, starting_age, victory);
}

StartMatch StartMatch::random(unsigned slave_count, unsigned player_count) {
	unsigned size = player_count <= 8 ? map_sizes[player_count] : (player_count + 6) * 12;
	// maps bigger than 65536*65536 are not supported, since we would need at least 4GB of RAM
	assert(size <= UINT16_MAX);
	
	StartMatch m{(uint8_t)rand(), 0, (uint16_t)size, (uint16_t)size, (uint32_t)rand(), (uint8_t)rand(), (uint8_t)rand(), 1, 1, (uint16_t)slave_count};
	m.dump();

	return m;
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

		if (type >= (uint16_t)CmdType::max || length != cmd_sizes[type])
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

	dbgf("read: buf=%p, len=%u\n", (void*)buf, len);
	while (len) {
		unsigned need;

		// wait for complete header before processing any data
		if (transmitted < CMD_HDRSZ) {
			dbgs("waiting for header data");
			need = CMD_HDRSZ - transmitted;

			if (need > len) {
				memcpy((char*)&cmd + transmitted, buf + processed, len);
				transmitted += len;
				processed += len;
				return 0; // stop, we need more data
			}

			dbgs("got header");

			memcpy((char*)&cmd + transmitted, buf + processed, need);
			transmitted += need;
			processed += need;
			len -= need;
		}

		// validate header
		unsigned type = be16toh(cmd.type), length = be16toh(cmd.length);

		if (type >= (uint16_t)CmdType::max || length != cmd_sizes[type]) {
			if (type < (uint16_t)CmdType::max)
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
			dbgf("need %u more bytes (%u available)\n", need, len);

			unsigned copy = need < len ? need : len;

			memcpy(((char*)&cmd) + transmitted, buf + processed, copy);
			transmitted += copy;
			len -= copy;
			processed += copy;
			dbgf("read %u bytes: trans, len: %u, %u\n", copy, transmitted, len);
		}

		// only process full packets
		if (transmitted >= size) {
			cmd.ntoh();

			dbgf("process: %d, %d\n", transmitted, size);

			dump((char*)&cmd, transmitted);
			putchar('\n');

			cb.event_process(endpoint, cmd);

			// move pending data to front
			memmove((char*)&cmd, ((char*)&cmd) + size, transmitted - size);

			// update state
			transmitted -= size;
			size = 0;

			dbgf("trans, size: %d, %d\n", transmitted, size);
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

void ServerSocket::broadcast(ServerCallback &cb, Command &cmd, bool net_order, bool ignore_bad) {
	if (!net_order)
		cmd.hton();

	std::lock_guard<std::recursive_mutex> lock(mut);

	for (auto &x : peers) {
		sockfd fd = pollfd(x);

		switch (push_unsafe(fd, cmd, true)) {
		case SSErr::OK: break;
		case SSErr::BADFD:
			if (!ignore_bad)
				removepeer(cb, fd);
			break;
		default:
			removepeer(cb, fd);
			break;
		}
	}
}

void ServerSocket::broadcast(ServerCallback &cb, Command &cmd, sockfd origfd, bool net_order) {
	if (!net_order)
		cmd.hton();

	std::lock_guard<std::recursive_mutex> lock(mut);

	push_unsafe(origfd, cmd, true);

	for (auto &x : peers) {
		sockfd fd = pollfd(x);

		if (fd == origfd)
			continue;

		switch (push_unsafe(fd, cmd, true)) {
		case SSErr::OK: break;
		default:
			removepeer(cb, fd);
			break;
		}
	}
}

bool operator<(const CmdBuf &lhs, const CmdBuf &rhs) {
	return lhs.endpoint < rhs.endpoint;
}

#if windows
#pragma warning(pop)
#endif

}
