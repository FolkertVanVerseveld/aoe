#pragma once

#include "net.hpp"

#include <thread>
#include <atomic>

namespace genie {

class Multiplayer {
protected:
	Net net;
	uint16_t port;
	std::thread t_worker;
public:
	Multiplayer(uint16_t port) : net(), port(port), t_worker() {}
	virtual ~Multiplayer() {}

	virtual void eventloop() = 0;
};

class MultiplayerHost final : public Multiplayer {
	ServerSocket sock;
public:
	MultiplayerHost(uint16_t port);
	~MultiplayerHost() override;

	void eventloop() override;
};

class MultiplayerClient final : public Multiplayer {
	Socket sock;
	std::atomic<bool> activated;
public:
	MultiplayerClient(uint16_t port);
	~MultiplayerClient() override;

	void eventloop() override;
};

}