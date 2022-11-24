#pragma once

#include "net.hpp"

#include <mutex>
#include <deque>
#include <vector>
#include <thread>

#include "game.hpp"

#if _WIN32
#include "../../wepoll/wepoll.h"
#endif

namespace aoe {

enum class NetPkgType {
	set_protocol,
	chat_text,
	start_game,
	set_scn_vars,
};

struct NetPkgHdr final {
	uint16_t type, payload;
	bool native_ordering;

	static constexpr size_t size = 4;

	NetPkgHdr(uint16_t type, uint16_t payload, bool native=true) : type(type), payload(payload), native_ordering(native) {}

	// byte swapping
	void ntoh();
	void hton();
};

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;

	static constexpr unsigned max_payload = UINT16_MAX - NetPkgHdr::size + 1;

	NetPkg() : hdr(0, 0, false), data() {}
	NetPkg(uint16_t type, uint16_t payload) : hdr(type, payload), data() {}
	/** munch as much data we need and check if valid. throws if invalid or not enough data. */
	NetPkg(std::deque<uint8_t> &q);

	void set_protocol(uint16_t version);
	uint16_t protocol_version();

	void set_chat_text(const std::string&);
	std::string chat_text();

	void set_start_game();

	void set_scn_vars(const ScenarioSettings &scn);
	ScenarioSettings get_scn_vars();

	NetPkgType type();

	void ntoh();
	void hton();

	void write(std::deque<uint8_t> &q);
	void write(std::vector<uint8_t> &q);

	size_t size() const noexcept {
		return NetPkgHdr::size + data.size();
	}
private:
	void set_hdr(NetPkgType type);
	void need_payload(size_t n);
};

class ClientInfo final {
public:
	std::string username;
};

class Server final : public ServerSocketController {
	ServerSocket s;
	std::atomic<bool> m_active;
	std::mutex m;
	uint16_t port, protocol;
	std::map<Peer, ClientInfo> peers;
public:
	Server();
	~Server();

	void stop();
	void close(); // this will block till everything is stopped

	bool active() const noexcept { return m_active; }

	int mainloop(int id, uint16_t port, uint16_t protocol);

	bool process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out);

	void incoming(ServerSocket &s, const Peer &p) override;
	void dropped(ServerSocket &s, const Peer &p) override;

	void stopped() override {}

	int proper_packet(ServerSocket &s, const std::deque<uint8_t> &q) override;
	bool process_packet(ServerSocket &s, const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed) override;
private:
	bool chk_protocol(const Peer &p, std::deque<uint8_t> &out, uint16_t req);

	void broadcast(NetPkg &pkg, bool include_host=true);
};

class Client final {
	TcpSocket s;
	std::string host;
	uint16_t port;
	std::atomic<bool> m_connected;
	std::mutex m;
public:
	Client();
	~Client();

	void start(const char *host, uint16_t port, bool run=true);
	void stop();

private:
	void mainloop();

	void add_chat_text(const std::string &s);
	void start_game();
	void set_scn_vars(const ScenarioSettings &scn);

public:
	bool connected() const noexcept { return m_connected; }

	template<typename T> void send(T *ptr, int len=1) {
		s.send_fully(ptr, len);
	}

	template<typename T> void recv(T *ptr, int len=1) {
		s.recv_fully(ptr, len);
	}

	void send(NetPkg&);
	NetPkg recv();

	// protocol api functions

	void send_protocol(uint16_t version);
	uint16_t recv_protocol();

	void send_chat_text(const std::string&);
	void send_start_game();

	void send_scn_vars(const ScenarioSettings &scn);
};

}
