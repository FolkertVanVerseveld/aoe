#pragma once

#include "../base/net.hpp"

#include <thread>
#include <atomic>
#include <set>
#include <string>
#include <mutex>
#include <queue>
#include <stack>

namespace genie {

extern void check_taunt(const std::string &str);

class Multiplayer {
protected:
	Net net;
	std::string name;
	uint16_t port;
	std::thread t_worker;
public:
	std::recursive_mutex mut; // lock for all following variables
	user_id id;
	std::queue<std::string> chats;

	Multiplayer(const std::string &name, uint16_t port);
	virtual ~Multiplayer() {}

	virtual void eventloop() = 0;

	virtual bool chat(const std::string &str, bool send=true) = 0;
};

class Slave final {
	sockfd fd;
public:
	user_id id;
	std::string name;

	Slave(sockfd fd) : fd(fd), id(0), name() {}
	Slave(sockfd fd, user_id id) : fd(fd), id(id), name() {}
	// serversocket only: for id == 0
	Slave(const std::string &name);

	friend bool operator<(const Slave &lhs, const Slave &rhs);
};

class MultiplayerHost final : public Multiplayer, protected ServerCallback {
	ServerSocket sock;
	std::set<Slave> slaves;
	std::map<user_id, sockfd> idmap;
	user_id idmod;
public:
	MultiplayerHost(const std::string &name, uint16_t port);
	~MultiplayerHost() override;

private:
	Slave &slave(sockfd fd);

	sockfd findfd(user_id id);
public:
	void eventloop() override;
	void incoming(pollev &ev) override;
	void removepeer(sockfd fd) override;
	void event_process(sockfd fd, Command &cmd) override;
	void shutdown() override;

	void dump();

	bool chat(const std::string &str, bool send=true) override;
};

class Peer final {
public:
	user_id id;
	std::string name;

	Peer(user_id id) : id(id), name() {}
	Peer(user_id id, const std::string &name) : id(id), name(name) {}

	friend bool operator<(const Peer &lhs, const Peer &rhs);
};

class MultiplayerClient final : public Multiplayer {
	Socket sock;
	uint32_t addr;
	std::atomic<bool> activated;
	user_id self;
	std::map<user_id, Peer> peers;
public:
	MultiplayerClient(const std::string &name, uint32_t addr, uint16_t port);
	~MultiplayerClient() override;

	void eventloop() override;
	bool chat(const std::string &str, bool send=true) override;
};

}
