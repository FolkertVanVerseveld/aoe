#pragma once

/*
 * Define network protocol application layer and low level server/client infrastructure.
 */

#include "../net/net.hpp"

#include <mutex>
#include <deque>
#include <vector>
#include <thread>
#include <variant>
#include <optional>

#include "game.hpp"
#include "debug.hpp"

#include <idpool.hpp>

#if _WIN32
#include <wepoll.h>
#endif

#include "net/protocol.hpp"

namespace aoe {

static_assert(sizeof(int) >= sizeof(int32_t));

class PkgWriter;

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;

	static constexpr unsigned max_payload = tcp4_max_size - NetPkgHdr::size;
	static constexpr unsigned ressize = 4 * sizeof(int32_t);

	friend PkgWriter;

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
	void set_gameover();

	void set_scn_vars(const ScenarioSettings&);
	ScenarioSettings get_scn_vars();

	void set_player_resize(size_t);
	void claim_player_setting(uint16_t); // client to server
	void claim_cpu_setting(uint16_t); // client to server
	void set_cpu_player(uint16_t); // server to client
	void set_player_civ(uint16_t, uint16_t);
	void set_player_team(uint16_t, uint16_t);
	void set_player_name(uint16_t, const std::string&);
	void set_player_died(uint16_t); // server to client
	NetPlayerControl get_player_control();

	void set_incoming(IdPoolRef);
	void set_dropped(IdPoolRef);
	void set_ref_username(IdPoolRef, const std::string&);
	void set_claim_player(IdPoolRef, uint16_t); // server to client
	NetPeerControl get_peer_control();

	void cam_set(float x, float y, float w, float h);
	NetCamSet get_cam_set();

	void set_terrain_mod(const NetTerrainMod&);
	NetTerrainMod get_terrain_mod();

	// TODO repurpose to playerview?
	void set_resources(const Resources&);
	Resources get_resources();

	// add is used to create entity without sfx
	void set_entity_add(const Entity&);
	void set_entity_add(const EntityView&);
	// spawn entity and make all sounds and particles needed for it
	void set_entity_spawn(const Entity&);
	void set_entity_spawn(const EntityView&);
	void set_entity_update(const Entity&);
	void set_entity_kill(IdPoolRef);
	void entity_move(IdPoolRef, float x, float y);
	void entity_task(IdPoolRef, IdPoolRef, EntityTaskType type=EntityTaskType::infer);
	NetEntityMod get_entity_mod();

	uint16_t get_gameticks();
	void set_gameticks(unsigned n);

	NetGamespeedControl get_gamespeed();
	void set_gamespeed(NetGamespeedType type);

	NetPkgType type();

	void ntoh();
	void hton();

	void write(std::deque<uint8_t> &q);
	void write(std::vector<uint8_t> &q);

	size_t size() const noexcept {
		return NetPkgHdr::size + data.size();
	}
private:
	void entity_add(const EntityView&, NetEntityControlType);
	void set_hdr(NetPkgType type);
	void need_payload(size_t n);

	void playermod2(NetPlayerControlType, uint16_t, uint16_t);
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

// TODO create world events and use them in world::event_queue
enum class WorldEventType {
	entity_add,
	entity_spawn,
	entity_kill,
	entity_task,
	player_kill,
	peer_cam_move,
	gameover,
	gamespeed_control,
};

class Server;

class EventCameraMove final {
public:
	IdPoolRef ref;
	NetCamSet cam;

	EventCameraMove(IdPoolRef ref, const NetCamSet &cam) : ref(ref), cam(cam) {}
};

class WorldEvent final {
public:
	WorldEventType type;
	std::variant<std::nullopt_t, IdPoolRef, Entity, EventCameraMove, EntityTask, NetGamespeedControl> data;

	template<class... Args> WorldEvent(WorldEventType type, Args&&... data) : type(type), data(data...) {}
};

class World;

/* Used for entity to query world info. */
class WorldView final {
	World &w;
public:
	WorldView(World &w) : w(w) {}

	Entity *try_get(IdPoolRef);
	bool try_convert(Entity&, Entity &aggressor);
};

class World final {
	std::mutex m, m_events;
	Terrain t;
	IdPool<Entity> entities;
	std::set<IdPoolRef> dirty_entities;
	std::vector<Player> players;
	std::deque<WorldEvent> events_in;
	std::map<IdPoolRef, NetCamSet> views; // display area for each peer
	Server *s;
	bool gameover;
	friend WorldView;
public:
	ScenarioSettings scn;
	std::atomic<double> logic_gamespeed;
	std::atomic<bool> running;

	static constexpr double gamespeed_max = 3.0;
	static constexpr double gamespeed_min = 0.5;
	static constexpr double gamespeed_step = 0.5;

	World();

	void load_scn(const ScenarioSettings &scn);

	NetTerrainMod fetch_terrain(int x, int y, unsigned &w, unsigned &h);

	void eventloop(Server &s);

	template<class... Args> void add_event(WorldEventType type, Args&&... data) {
		std::lock_guard<std::mutex> lk(m_events);
		events_in.emplace_back(type, data...);
	}
private:
	void startup();
	void create_terrain();
	void create_players();
	void create_entities();

	void add_building(EntityType t, unsigned player, int x, int y);
	void add_unit(EntityType t, unsigned player, float x, float y, float angle=0, EntityState state=EntityState::alive);
	void add_resource(EntityType t, float x, float y);

	void tick();
	void tick_entities();
	void tick_players();
	void pump_events();
	void push_events();
	void cam_move(WorldEvent&);

	void gamespeed_control(WorldEvent&);

	void send_gameticks(unsigned);

	bool single_team() const noexcept;

	void stop();

	void entity_kill(WorldEvent &ev);
	void entity_task(WorldEvent &ev);
};

class Server final : public ServerSocketController {
	ServerSocket s;
	std::atomic<bool> m_active, m_running;
	std::mutex m_peers;
	uint16_t port, protocol;
	std::map<Peer, ClientInfo> peers;
	IdPool<SocketRef> refs;
	// TODO move these into a world class or smth
	World w;
	std::map<std::string, std::vector<std::string>> civs;
	std::vector<std::string> civnames;

	friend Debug;
	friend World;
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
	bool process_entity_mod(const Peer &p, NetEntityMod &em, std::deque<uint8_t> &out);

	bool cam_set(const Peer &p, NetCamSet &cam);

	void gamespeed_control(const NetGamespeedControl &control);

	void start_game();

	ClientInfo &get_ci(IdPoolRef);

	void broadcast(NetPkg &pkg, bool include_host=true);
	void broadcast(NetPkg &pkg, const Peer &exclude);
	void send(const Peer &p, NetPkg &pkg);
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
	std::atomic<bool> m_connected, starting;
	std::mutex m;

	std::map<IdPoolRef, ClientInfo> peers;
	IdPoolRef me;
	ScenarioSettings scn;
	unsigned modflags;
	std::atomic<bool> gameover;
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
	void entitymod(const NetEntityMod&);
	void gameticks(unsigned n);
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
	void send_set_player_civ(unsigned idx, unsigned civ);
	void send_set_player_team(unsigned idx, unsigned team);

	void send_scn_vars(const ScenarioSettings &scn);
	void send_username(const std::string&);

	void claim_player(unsigned);
	void claim_cpu(unsigned);

	void cam_move(float x, float y, float w, float h);

	void send_gamespeed_control(const NetGamespeedControl&);

	void entity_move(IdPoolRef, float x, float y);
	void entity_infer(IdPoolRef, IdPoolRef);

	/** Try to destroy entity. */
	void entity_kill(IdPoolRef);
};

class ClientView final {
public:
	IdPoolRef me;
	ScenarioSettings scn;
	bool gameover;

	ClientView();

	bool try_read(Client&);
};

}
