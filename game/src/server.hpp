#pragma once

/*
 * Define network protocol application layer and low level server/client infrastructure.
 */

#include "net.hpp"

#include <mutex>
#include <deque>
#include <vector>
#include <thread>
#include <variant>
#include <optional>

#include "game.hpp"
#include "debug.hpp"
#include "idpool.hpp"

#if _WIN32
#include <wepoll.h>
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
	terrainmod,
	resmod,
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
	set_ref,
	set_cpu_ref,
	set_player_name,
};

enum class NetPeerControlType {
	incoming,
	dropped,
	set_username,
	set_player_idx,
};

class NetPlayerControl final {
public:
	NetPlayerControlType type;
	std::variant<std::nullopt_t, IdPoolRef, uint16_t, std::pair<uint16_t, std::string>> data;

	static constexpr unsigned resize_size = 2 * sizeof(uint16_t);

	NetPlayerControl() : type(NetPlayerControlType::resize), data(std::nullopt) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t arg) : type(type), data(arg) {}
	NetPlayerControl(NetPlayerControlType type, IdPoolRef ref) : type(type), data(ref) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t idx, const std::string &name) : type(type), data(std::make_pair(idx, name)) {}
};

class NetPeerControl final {
public:
	IdPoolRef ref;
	NetPeerControlType type;
	std::variant<std::nullopt_t, std::string, uint16_t> data;

	NetPeerControl(IdPoolRef ref, NetPeerControlType type) : ref(ref), type(type), data(std::nullopt) {}
	NetPeerControl(IdPoolRef ref, const std::string &name) : ref(ref), type(NetPeerControlType::set_username), data(name) {}
	NetPeerControl(IdPoolRef ref, NetPeerControlType type, uint16_t idx) : ref(ref), type(type), data(idx) {}
};

class NetTerrainMod final {
public:
	uint16_t x, y, w, h;
	std::vector<uint8_t> tiles;
	std::vector<int8_t> hmap;

	static constexpr size_t possize = 4 * sizeof(uint16_t);

	NetTerrainMod() : x(0), y(0), w(0), h(0), tiles(), hmap() {}
};

static_assert(sizeof(int) >= sizeof(int32_t));

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;

	static constexpr unsigned max_payload = tcp4_max_size - NetPkgHdr::size;
	static constexpr unsigned ressize = 4 * sizeof(int32_t);

	NetPkg() : hdr(0, 0, false), data() {}
	NetPkg(uint16_t type, uint16_t payload) : hdr(type, payload), data() {}
	/** munch as much data we need and check if valid. throws if invalid or not enough data. */
	NetPkg(std::deque<uint8_t> &q);

	void set_protocol(uint16_t version);
	uint16_t protocol_version();

	void set_chat_text(IdPoolRef, const std::string&);
	std::pair<IdPoolRef, std::string> chat_text();

	void set_username(const std::string&);
	std::string username();

	void set_start_game();

	void set_scn_vars(const ScenarioSettings&);
	ScenarioSettings get_scn_vars();

	void set_player_resize(size_t);
	void claim_player_setting(uint16_t); // client to server
	void claim_cpu_setting(uint16_t); // client to server
	void set_cpu_player(uint16_t); // server to client
	void set_player_name(uint16_t, const std::string&);
	NetPlayerControl get_player_control();

	void set_incoming(IdPoolRef);
	void set_dropped(IdPoolRef);
	void set_ref_username(IdPoolRef, const std::string&);
	void set_claim_player(IdPoolRef, uint16_t); // server to client
	NetPeerControl get_peer_control();

	void set_terrain_mod(const NetTerrainMod&);
	NetTerrainMod get_terrain_mod();

	void set_resources(const Resources&);
	Resources get_resources();

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

/*
 * Simple wrapper to make peers uniquely identifiable without leaking the ip
 * address and using a fixed width identifier to reduce network bandwidth.
 */
class SocketRef final {
public:
	IdPoolRef ref;
	SOCKET sock;

	SocketRef(IdPoolRef ref, SOCKET sock) : ref(ref), sock(sock) {}
};

class Server final : public ServerSocketController {
	ServerSocket s;
	std::atomic<bool> m_active, m_running;
	std::mutex m_peers;
	uint16_t port, protocol;
	std::map<Peer, ClientInfo> peers;
	IdPool<SocketRef> refs;
	ScenarioSettings scn;
	std::atomic<double> logic_gamespeed;
	Terrain t;

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

	void start_game();

	ClientInfo &get_ci(IdPoolRef);

	void broadcast(NetPkg &pkg, bool include_host=true);
	void broadcast(NetPkg &pkg, const Peer &exclude);
	void send(const Peer &p, NetPkg &pkg);

	void eventloop();
	void tick();
};

class ClientView;

enum class ClientModFlags {
	scn = 1 << 0,
	terrain = 1 << 1,
};

class Client final {
	TcpSocket s;
	std::string host;
	uint16_t port;
	std::atomic<bool> m_connected;
	std::mutex m;

	std::map<IdPoolRef, ClientInfo> peers;
	IdPoolRef me;
	ScenarioSettings scn;
	unsigned modflags;
	friend Debug;
	friend ClientView;
public:
	Game g;

	Client();
	~Client();

	void start(const char *host, uint16_t port, bool run=true);
	void stop();
private:
	void mainloop();

	void add_chat_text(IdPoolRef, const std::string &s);
	void start_game();
	void set_scn_vars(const ScenarioSettings &scn);
	void set_username(const std::string &s);
	void playermod(const NetPlayerControl&);
	void peermod(const NetPeerControl&);
	void terrainmod(const NetTerrainMod&);
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
	void send_set_player_name(unsigned idx, const std::string&);

	void send_scn_vars(const ScenarioSettings &scn);
	void send_username(const std::string&);

	void claim_player(unsigned);
	void claim_cpu(unsigned);
};

class ClientView final {
public:
	IdPoolRef me;
	ScenarioSettings scn;

	ClientView();

	bool try_read(Client&);
};

}
