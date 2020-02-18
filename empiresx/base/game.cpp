#include "game.hpp"

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

MultiplayerHost::MultiplayerHost(uint16_t port) : Multiplayer(port), sock(port), slaves() {
	puts("start host");
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

void MultiplayerHost::eventloop() {
	printf("host started on 127.0.0.1:%" PRIu16 "\n", port);
	sock.eventloop(*this);
}

void MultiplayerHost::event_process(sockfd fd, Command &cmd) {
	switch (cmd.type) {
	case CmdType::TEXT:
		sock.broadcast(cmd);
		{
			auto str = cmd.text();
			std::lock_guard<std::mutex> lock(mut);
			chats.emplace(str);
			check_taunt(str);
		}
		break;
	}
}

void MultiplayerHost::incoming(pollev& ev) {
	slaves.emplace(pollfd(ev));
}

void MultiplayerHost::removepeer(sockfd fd) {
	slaves.erase(fd);
}

void MultiplayerHost::shutdown() {
	puts("host shutdown");
}

void MultiplayerHost::chat(const std::string &str) {
	Command txt = Command::text(str);
	sock.broadcast(txt);

	std::lock_guard<std::mutex> lock(mut);
	chats.emplace(str);
	check_taunt(str);
}

MultiplayerClient::MultiplayerClient(uint32_t addr, uint16_t port) : Multiplayer(port), sock(port), addr(addr), activated(false) {
	sock.reuse();

	t_worker = std::thread(client_start, std::ref(*this));
}

MultiplayerClient::~MultiplayerClient() {
	puts("closing client");
	// NOTE sock.close() has to be in both bodies, because its behavior will be slightly different!
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
		fputs("failed to connect\n", stderr);
		return;
	}

	puts("connected");

	for (activated.store(true); activated.load();) {
		Command cmd;

		if (sock.recv(cmd)) {
			activated.store(false);
			continue;
		}

		switch (cmd.type) {
		case TEXT:
			{
				std::lock_guard<std::mutex> lock(mut);
				auto str = cmd.text();
				chats.emplace(str.c_str());
				check_taunt(str);
			}
			break;
		default:
			fprintf(stderr, "communication error: unknown type %u\n", cmd.type);
			activated.store(false);
			continue;
		}
	}
}

void MultiplayerClient::chat(const std::string &str) {
	Command cmd = Command::text(str);
	sock.send(cmd, false);
}

}
