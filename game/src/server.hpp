#pragma once

/*
 * Define network protocol application layer and low level server/client infrastructure.
 */

#include "../net/net.hpp"

#include <mutex>
#include <deque>
#include <vector>
#include <cstdint>
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
#include "net/netpkg.hpp"

#include "net/clientinfo.hpp"

#include "world/world.hpp"

#include "async.hpp"

namespace aoe {

typedef std::lock_guard<std::mutex> lock; // easier to read

static_assert(sizeof(int) >= sizeof(int32_t));
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

class WorldEvent final {
public:
	IdPoolRef src; /** ref to peer that created this event. invalid_ref if from the server itself. */
	WorldEventType type;
	std::variant<std::nullopt_t, IdPoolRef, Entity, EventCameraMove, EntityTask, NetGamespeedControl> data;

	template<class... Args> WorldEvent(IdPoolRef src, WorldEventType type, Args&&... data) : src(src), type(type), data(data...) {}
};

class World;

/* Used for entity to query world info. */
class WorldView final {
	World &w;
public:
	WorldView(World &w) : w(w) {}

	Entity &at(IdPoolRef);
	Entity *try_get(IdPoolRef);
	Entity *try_get_alive(float x, float y, EntityType);
	bool try_convert(Entity&, Entity &aggressor);
	void collect(unsigned player, const Resources &res);
};

class IServer;

class World final {
	std::mutex m, m_events;
	Terrain t;
	IdPool<Entity> entities;
	std::set<IdPoolRef> dirty_entities, spawned_entities, died_entities, killed_entities;
	IdPool<Particle> particles;
	std::set<IdPoolRef> spawned_particles;
	std::vector<Player> players;
	std::vector<PlayerAchievements> player_achievements;
	std::deque<WorldEvent> events_in, events_out;
	std::map<IdPoolRef, NetCamSet> views; // display area for each peer
	std::set<unsigned> resources_out;
	IServer *s;
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

	void eventloop(IServer &s, UI_TaskInfo *info);

	template<class... Args> void add_event(IdPoolRef src, WorldEventType type, Args&&... data) {
		std::lock_guard<std::mutex> lk(m_events);
		events_in.emplace_back(src, type, data...);
	}

	int non_gaia_players() const noexcept { return this->players.size() - first_player_idx; }
private:
	void startup();
	void create_terrain();
	void create_players();
	void create_entities();

	void add_building(EntityType t, unsigned player, int x, int y);
	void add_unit(EntityType t, unsigned player, float x, float y);
	void add_unit(EntityType t, unsigned player, float x, float y, float angle, EntityState state=EntityState::alive);
	void add_resource(EntityType t, float x, float y, unsigned subimage);
	void add_berries(float x, float y);
	void add_gold(float x, float y);
	void add_stone(float x, float y);

	void spawn_unit(EntityType t, unsigned player, float x, float y);
	void spawn_unit(EntityType t, unsigned player, float x, float y, float angle);
	void spawn_particle(ParticleType t, float x, float y);

	void tick();
	void tick_entities();
	void tick_particles();
	void tick_players();
	void pump_events();
	void push_events();

	void push_entities();
	void push_particles();
	void push_scores();
	void push_resources();

	void cam_move(WorldEvent&);

	void gamespeed_control(WorldEvent&);
	void push_gamespeed_control(WorldEvent&);

	void send_gameticks(unsigned);

	bool single_team() const noexcept;

	void stop(unsigned);

	void entity_kill(WorldEvent &ev);
	void entity_task(WorldEvent &ev);

	void nuke_ref(IdPoolRef);

	bool controls_player(IdPoolRef src, unsigned pid);

	void save_scores();
	void send_scores();

	void send_resources();

	void send_player(unsigned i, NetPkg &pkg);

	std::optional<unsigned> ref2idx(IdPoolRef) const noexcept;
};

class IServer {
protected:
	std::atomic<bool> m_active;
public:
	IServer() : m_active(false) {}

	virtual ~IServer() = default;

	bool active() const noexcept { return m_active; }

	virtual bool is_running() const noexcept =0;
	virtual void close() =0;

	virtual void broadcast(NetPkg &pkg, bool include_host=true) =0;
};

class Server final : public ServerSocketController, public IServer {
	ServerSocket s;
	std::atomic<bool> m_running;
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
	~Server() override;

	void stop();
	void close() override; // this will block till everything is stopped

	int mainloop(uint16_t port, uint16_t protocol, bool testing=false);

	bool process(const Peer &p, NetPkg &pkg, std::deque<uint8_t> &out);

	bool incoming(ServerSocket &s, const Peer &p) override;
	void dropped(ServerSocket &s, const Peer &p) override;

	void stopped() override;

	int proper_packet(ServerSocket &s, const std::deque<uint8_t> &q) override;
	bool process_packet(ServerSocket &s, const Peer &p, std::deque<uint8_t> &in, std::deque<uint8_t> &out, int processed) override;

	bool is_running() const noexcept override { return m_running.load(); }
private:
	bool chk_protocol(const Peer &p, std::deque<uint8_t> &out, NetPkg &pkg);
	bool chk_username(const Peer &p, std::deque<uint8_t> &out, const std::string &name);

	void change_username(const Peer &p, std::deque<uint8_t> &out, const std::string &name);
	bool set_scn_vars(const Peer &p, ScenarioSettings &scn);

	bool process_clientinfo(const Peer &p, NetPkg &pkg);
	bool process_playermod(const Peer &p, NetPlayerControl &ctl, std::deque<uint8_t> &out);
	bool process_entity_mod(const Peer &p, NetEntityMod &em, std::deque<uint8_t> &out);

	bool cam_set(const Peer &p, NetCamSet &cam);

	void gamespeed_control(const Peer &p, const NetGamespeedControl &control);

	void start_game(const Peer &p);

	const Peer *try_peer(IdPoolRef);
	ClientInfo &get_ci(IdPoolRef);

	void broadcast(NetPkg &pkg, bool include_host=true) override;
	void broadcast(NetPkg &pkg, const Peer &exclude);
	void send(const Peer &p, NetPkg &pkg);

	IdPoolRef peer2ref(const Peer&);
};

class ClientView;

enum class ClientModFlags {
	scn = 1 << 0,
	ref = 1 << 1,
	terrain = 1 << 2,
};

class IClient {
protected:
	std::mutex m;

	IdPoolRef me;
	ScenarioSettings scn;
	unsigned modflags;

	unsigned playerindex, team_me;
	bool victory;
	std::atomic<bool> gameover;
	std::atomic<bool> m_connected;

	friend ClientView;
public:
	IClient();
	virtual ~IClient() = default;

	virtual bool is_running() const noexcept =0;
	bool connected() const noexcept { return m_connected; }

	virtual void stop() =0;

	virtual void claim_player(unsigned) =0;
	virtual void cam_move(float x, float y, float w, float h) =0;
	virtual void send_gamespeed_control(NetGamespeedType type) =0;

	virtual void send_chat_text(const std::string&) =0;
	virtual void send_start_game() =0;
	virtual void send_ready(bool) =0;
	virtual void send_players_resize(unsigned n) =0;

	virtual void send_set_player_name(unsigned idx, const std::string&) =0;
	virtual void send_set_player_civ(unsigned idx, unsigned civ) =0;
	virtual void send_set_player_team(unsigned idx, unsigned team) =0;

	virtual void send_scn_vars(const ScenarioSettings &scn) =0;
	virtual void send_username(const std::string&) =0;

	virtual void entity_move(IdPoolRef, float x, float y) =0;
	virtual void entity_infer(IdPoolRef, IdPoolRef) =0;
	virtual void entity_train(IdPoolRef, EntityType) =0;
	virtual void entity_kill(IdPoolRef) =0;
};

class LocalClient final : public IClient {
public:
	World w;

	LocalClient();

	bool is_running() const noexcept override { return w.running; }
	void stop() override;

	void create_game(const ScenarioSettings &scn, UI_TaskInfo &info);

	void send_chat_text(const std::string&) override;
	void send_start_game() override;
	void send_ready(bool) override;
	void send_players_resize(unsigned n) override;
	void send_set_player_name(unsigned idx, const std::string&) override;
	void send_set_player_civ(unsigned idx, unsigned civ) override;
	void send_set_player_team(unsigned idx, unsigned team) override;

	void send_scn_vars(const ScenarioSettings &scn) override;
	void send_username(const std::string&) override;

	void claim_player(unsigned) override;
	void cam_move(float x, float y, float w, float h) override;
	void send_gamespeed_control(NetGamespeedType type) override;

	void entity_move(IdPoolRef, float x, float y) override;
	void entity_infer(IdPoolRef, IdPoolRef) override;
	void entity_train(IdPoolRef, EntityType) override;

	/** Try to destroy entity. */
	void entity_kill(IdPoolRef) override;
};

class Client final : public IClient {
	TcpSocket s;
	std::string host;
	uint16_t port;
	std::atomic<bool> starting;

	std::map<IdPoolRef, ClientInfo> peers;
	friend Debug;
public:
	Game g;

	Client();
	~Client() override {
		stop();
	}

	bool is_running() const noexcept override { return g.running; }

	void start(const char *host, uint16_t port, bool run=true);
	void stop() override;
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
	void particlemod(NetPkg&);
	void resource_ctl(NetPkg&);
	void gameticks(unsigned n);
	void gamespeed_control(const NetGamespeedControl&);

	void set_me(IdPoolRef);
public:
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

	void send_chat_text(const std::string&) override;
	void send_start_game() override;
	void send_ready(bool) override;
	void send_players_resize(unsigned n) override;
	void send_set_player_name(unsigned idx, const std::string&) override;
	void send_set_player_civ(unsigned idx, unsigned civ) override;
	void send_set_player_team(unsigned idx, unsigned team) override;

	void send_scn_vars(const ScenarioSettings &scn) override;
	void send_username(const std::string&) override;

	void claim_player(unsigned) override;
	void cam_move(float x, float y, float w, float h) override;
	void send_gamespeed_control(NetGamespeedType type) override;

	void entity_move(IdPoolRef, float x, float y) override;
	void entity_infer(IdPoolRef, IdPoolRef) override;
	void entity_train(IdPoolRef, EntityType) override;

	/** Try to destroy entity. */
	void entity_kill(IdPoolRef) override;
};

class ClientView final {
public:
	IdPoolRef me;
	ScenarioSettings scn;
	bool gameover, victory;
	unsigned playerindex;

	ClientView();

	bool try_read(IClient&);
};

/*
 * since i've made the mistake multiple times to forget to call set_hdr, we can use this wrapper to take care of that.
 */
class PkgWriter final {
public:
	NetPkg &pkg;
	const NetPkgType type;

	PkgWriter(NetPkg &pkg, NetPkgType t, size_t n=0) : pkg(pkg), type(t) {
		if (n)
			pkg.data.resize(n);
	}

	~PkgWriter() {
		pkg.set_hdr(type);
	}
};

}
