#include "game.hpp"

#include <cassert>
#include <cstdio>
#include <inttypes.h>

#include "../string.hpp"

#include <string>
#include <map>

namespace genie {

void host_start(MultiplayerHost& host) {
	host.eventloop();
}

void client_start(MultiplayerClient& client) {
	puts("start client");
	client.eventloop();
}

bool operator<(const Slave& lhs, const Slave& rhs) {
	return lhs.fd < rhs.fd;
}

bool operator<(const Peer &lhs, const Peer &rhs) {
	return lhs.id < rhs.id;
}

Slave::Slave(const std::string &name) : fd(INVALID_SOCKET), id(0), name(name) {}

Multiplayer::Multiplayer(MultiplayerCallback &cb, const std::string &name, uint16_t port)
	: net(), name(name), port(port), t_worker(), cb(cb), mut(), chats() {}

MultiplayerHost::MultiplayerHost(MultiplayerCallback &cb, const std::string &name, uint16_t port)
	: Multiplayer(cb, name, port), sock(port), slaves(), idmap(), idmod(1)
{
	puts("start host");
	// claim slot for server itself: id == 0 is used for that purpose
	slaves.emplace(name);
	t_worker = std::thread(host_start, std::ref(*this));
}

MultiplayerHost::~MultiplayerHost() {
	puts("closing host");
	sock.close();
#if linux
	pthread_cancel(t_worker.native_handle());
#endif
	t_worker.join();
	puts("host stopped");
}

Slave &MultiplayerHost::slave(sockfd fd) {
	auto search = slaves.find(fd);
	if (search == slaves.end())
		throw std::runtime_error(std::string("bad slave fd: ") + std::to_string(fd));

	return const_cast<Slave&>(*search);
}

void MultiplayerHost::eventloop() {
	printf("host started on 127.0.0.1:%" PRIu16 "\n", port);
	sock.eventloop(*this);
}

void MultiplayerHost::event_process(sockfd fd, Command &cmd) {
	switch ((CmdType)cmd.type) {
	case CmdType::TEXT:
		sock.broadcast(*this, cmd);
		{
			auto str = cmd.text();
			std::lock_guard<std::recursive_mutex> lock(mut);
			chats.emplace(str);
			check_taunt(str);
		}
		break;
	case CmdType::JOIN:
		{
			auto join = cmd.join();
#if windows
			printf("%llu joins as %s\n", fd, join.nick().c_str());
#else
			printf("%d joins as %s\n", fd, join.nick().c_str());
#endif

			Slave &s = slave(fd);
			s.name = join.nick().c_str();

			printf("id: %u\n", s.id);
			chats.emplace(join.nick() + " has joined");

			Command cmd = Command::join(s.id, s.name);
			cmd.hton();

			// always send back first to the slave it came from
			sock.broadcast(*this, cmd, fd, true);

			// send all joined slaves to new client
			for (auto &x : slaves) {
				// ignore special slave and client itself
				if ((x.id == 0 && x.name.empty()) || x.id == s.id)
					continue;
				Command cmd = Command::join(x.id, x.name);
				sock.push(fd, cmd, false);
			}

			cb.join(join);
		}
		break;
	}
}

void MultiplayerHost::incoming(pollev &ev) {
	std::lock_guard<std::recursive_mutex> lock(mut);
	// disallow id 0 as slave, because this is always the host itself
	if (idmod == 0)
		++idmod;

	slaves.emplace(pollfd(ev), idmod++);
}

void MultiplayerHost::removepeer(sockfd fd) {
	std::lock_guard<std::recursive_mutex> lock(mut);

	Slave &s = slave(fd);
	user_id leave = s.id;
	assert(leave);
	printf("%s has left\n", s.name.c_str());
	chats.emplace(s.name + " has left");

	slaves.erase(fd);

	Command cmd = Command::leave(leave);
	sock.broadcast(*this, cmd, false, true);
}

void MultiplayerHost::shutdown() {
	puts("host shutdown");
	std::lock_guard<std::recursive_mutex> lock(mut);
	slaves.clear();
}

void MultiplayerHost::dump() {
	std::lock_guard<std::recursive_mutex> lock(mut);
	printf("slaves: %lu\n", (long unsigned)slaves.size());

	for (auto &x : slaves)
		printf("%u %s\n", x.id, x.name.c_str());
}

bool MultiplayerHost::chat(const std::string &str, bool send) {
	if (send) {
		Command txt = Command::text(str);
		sock.broadcast(*this, txt);
	}

	std::lock_guard<std::recursive_mutex> lock(mut);
	chats.emplace(str);
	check_taunt(str);
	return true;
}

MultiplayerClient::MultiplayerClient(MultiplayerCallback &cb, const std::string &name, uint32_t addr, uint16_t port)
	: Multiplayer(cb, name, port), sock(port), addr(addr), activated(false), self(0), peers()
{
	sock.reuse();

	t_worker = std::thread(client_start, std::ref(*this));
}

MultiplayerClient::~MultiplayerClient() {
	puts("closing client");
	// NOTE sock.close() has to be in both bodies, because its behavior will be slightly different!
	// XXX figure out how to force a memory barrier between joinable and close or verify that the generated assembly is correct
	if (t_worker.joinable()) {
		sock.close();
#if linux
		pthread_cancel(t_worker.native_handle());
#endif
		t_worker.join();
	} else {
		sock.close();
	}
	puts("client stopped");
}

void MultiplayerClient::eventloop() {
	bool connected = false;

	// FIXME improve connection establishment tolerance
	/**
	 * het idee is (de volgende punten gecombineerd):
	 * - probeer een X aantal keer te verbinden, maar altijd minstens 2 keer
	 * - als een connect poging mislukt, kijk hoeveel tijd er sinds connect verstreken is.
	 *   als het minder dan 500ms was, wacht totdat er ongeveer intotaal 500ms is verstreken
	 * - geef pas op als er minimaal Y ms zijn verstreken
	 */

	for (unsigned tries = 3; tries && !connected; --tries) {
		int err;

		if (!(err = sock.connect(addr, true))) {
			connected = true;
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	if (!connected) {
		chat("Failed to connect", false);
		return;
	}

	chat("Connected to server", false);

	// send desired nickname
	Command cmd = Command::join(0, name);
	sock.send(cmd, false);

	// main loop
	for (activated.store(true); activated.load();) {
		Command cmd;

		if (sock.recv(cmd)) {
			activated.store(false);
			chat("Server stopped", false);
			continue;
		}

		switch ((CmdType)cmd.type) {
		case CmdType::TEXT:
			{
				std::lock_guard<std::recursive_mutex> lock(mut);
				auto str = cmd.text();
				chats.emplace(str.c_str());
				check_taunt(str);
			}
			break;
		case CmdType::JOIN:
			{
				std::lock_guard<std::recursive_mutex> lock(mut);
				JoinUser usr = cmd.join();
				const std::string &s = usr.nick();

				chats.emplace(s + " has joined");

				if (self == 0) {
					self = usr.id;
					name = s;
					printf("joined as %u: %s\n", usr.id, s.c_str());
				}

				peers.emplace(std::piecewise_construct, std::forward_as_tuple(usr.id), std::forward_as_tuple(usr.id, s));
				cb.join(usr);
			}
			break;
		case CmdType::LEAVE:
			{
				std::lock_guard<std::recursive_mutex> lock(mut);
				user_id leave = cmd.data.leave;

				if (leave == self) {
					fputs("we got kicked!\n", stderr);
					chats.emplace("You are kicked from the server");
					activated.store(false);
					continue;
				}

				auto search = peers.find(leave);

				if (search != peers.end())
					chats.emplace(search->second.name + " has left");

				peers.erase(leave);
			}
			break;
		default:
			fprintf(stderr, "communication error: unknown type %u\n", cmd.type);
			chats.emplace("Communication error");
			activated.store(false);
			continue;
		}
	}

	sock.close();
}

bool MultiplayerClient::chat(const std::string &str, bool send) {
	if (!send || !activated.load()) {
		std::lock_guard<std::recursive_mutex> lock(mut);
		chats.emplace(send ? "Not connected to server" : str);
		return false;
	}

	Command cmd = Command::text(str);
	sock.send(cmd, false);
	return true;
}

}
