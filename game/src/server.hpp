#pragma once

#include "net.hpp"

#include <mutex>
#include <deque>
#include <vector>
#include <thread>
#include <variant>

#include "game.hpp"
#include "debug.hpp"
#include "idpool.hpp"

#if _WIN32
#include "../../wepoll/wepoll.h"
#endif

namespace aoe {

enum class NetPkgType {
	set_protocol,
	chat_text,
	start_game,
	set_scn_vars,
	set_username,
	playermod,
	peermod,
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

enum class NetPlayerControlType {
	resize,
	erase,
};

enum class NetPeerControlType {
	incoming,
	dropped,
};

class NetPlayerControl final {
public:
	NetPlayerControlType type;
	uint16_t arg;

	NetPlayerControl() : type(NetPlayerControlType::resize), arg(0) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t arg) : type(type), arg(arg) {}
};

class NetPeerControl final {
public:
	IdPoolRef ref;
	NetPeerControlType type;

	NetPeerControl(IdPoolRef ref, NetPeerControlType type) : ref(ref), type(type) {}
};

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;

	static constexpr unsigned max_payload = tcp4_max_size - NetPkgHdr::size;

	NetPkg() : hdr(0, 0, false), data() {}
	NetPkg(uint16_t type, uint16_t payload) : hdr(type, payload), data() {}
	/** munch as much data we need and check if valid. throws if invalid or not enough data. */
	NetPkg(std::deque<uint8_t> &q);

	void set_protocol(uint16_t version);
	uint16_t protocol_version();

	void set_chat_text(const std::string&);
	std::string chat_text();

	void set_username(const std::string&);
	std::string username();

	void set_start_game();

	void set_scn_vars(const ScenarioSettings&);
	ScenarioSettings get_scn_vars();

	void set_player_resize(size_t);
	NetPlayerControl get_player_control();

	void set_incoming(IdPoolRef);
	void set_dropped(IdPoolRef);
	NetPeerControl get_peer_control();

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

enum class ClientInfoFlags {
	ready = 1 << 0,
};

class ClientInfo final {
public:
	std::string username;
	unsigned flags;
	IdPoolRef ref;

	ClientInfo() : username(), flags(0), ref(invalid_ref) {}
	ClientInfo(IdPoolRef ref, const std::string &username) : username(username), flags(0), ref(ref) {}
};

class SocketRef final {
public:
	IdPoolRef ref;
	SOCKET sock;

	SocketRef(IdPoolRef ref, SOCKET sock) : ref(ref), sock(sock) {}
};

class Server final : public ServerSocketController {
	ServerSocket s;
	std::atomic<bool> m_active;
	std::mutex m_peers;
	uint16_t port, protocol;
	std::map<Peer, ClientInfo> peers;
	IdPool<SocketRef> refs;
	ScenarioSettings scn;

	friend Debug;
public:
	Server();
	~Server();

	void stop();
	void close(); // this will block till everything is stopped

	bool active() const noexcept { return m_active; }

	int mainloop(int id, uint16_t port, uint16_t protocol);

	bool process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out);

	bool incoming(ServerSocket &s, const Peer &p) override;
	void dropped(ServerSocket &s, const Peer &p) override;

	void stopped() override;

	int proper_packet(ServerSocket &s, const std::deque<uint8_t> &q) override;
	bool process_packet(ServerSocket &s, const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed) override;
private:
	bool chk_protocol(const Peer &p, std::deque<uint8_t> &out, uint16_t req);
	bool chk_username(const Peer &p, std::deque<uint8_t> &out, const std::string &name);

	void change_username(const Peer &p, std::deque<uint8_t> &out, const std::string &name);
	bool set_scn_vars(const Peer &p, ScenarioSettings &scn);

	bool process_playermod(const Peer &p, NetPlayerControl &ctl, std::deque<uint8_t> &out);

	void broadcast(NetPkg &pkg, bool include_host=true);
	void send(const Peer &p, NetPkg &pkg);
};

class Client final {
	TcpSocket s;
	std::string host;
	uint16_t port;
	std::atomic<bool> m_connected;
	std::mutex m;

	std::map<IdPoolRef, ClientInfo> peers;
	IdPoolRef me;
	friend Debug;
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
	void set_username(const std::string &s);
	void playermod(const NetPlayerControl&);
	void peermod(const NetPeerControl&);
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
	void send_players_resize(unsigned n);

	void send_scn_vars(const ScenarioSettings &scn);
	void send_username(const std::string&);
};

}
