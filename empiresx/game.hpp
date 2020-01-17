#pragma once

#include "net.hpp"

#include <thread>
#include <atomic>
#include <set>

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

class Slave final {
	sockfd fd;
public:
	Slave(sockfd fd) : fd(fd) {}

	friend bool operator<(const Slave& lhs, const Slave& rhs);
};

class MultiplayerHost final : public Multiplayer, protected ServerCallback {
	ServerSocket sock;
	std::set<Slave> slaves;
public:
	MultiplayerHost(uint16_t port);
	~MultiplayerHost() override;

	void eventloop() override;
	void incoming(pollev &ev) override;
	void removepeer(sockfd fd) override;
	void event_process(sockfd fd, Command &cmd) override;
	void shutdown() override;
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
