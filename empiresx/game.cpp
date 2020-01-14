#include "game.hpp"

#include <cstdio>
#include <inttypes.h>

namespace genie {

void host_start(MultiplayerHost& host) {
	host.eventloop();
}

void client_start(MultiplayerClient& client) {
	puts("start client");
	client.eventloop();
}

MultiplayerHost::MultiplayerHost(uint16_t port) : Multiplayer(port), sock(port) {
	puts("start host");
	t_worker = std::thread(host_start, std::ref(*this));
}

MultiplayerHost::~MultiplayerHost() {
	puts("closing host");
	sock.close();
	t_worker.join();
	puts("host stopped");
}

void MultiplayerHost::eventloop() {
	printf("host started on 127.0.0.1:%" PRIu16 "\n", port);
	sock.eventloop();
}

MultiplayerClient::MultiplayerClient(uint16_t port) : Multiplayer(port), sock(port), activated(false) {
	sock.reuse();

	t_worker = std::thread(client_start, std::ref(*this));
}

MultiplayerClient::~MultiplayerClient() {
	if (t_worker.joinable()) {
		sock.close();
		t_worker.join();
	}
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

		if (!(err = sock.connect())) {
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
}

}