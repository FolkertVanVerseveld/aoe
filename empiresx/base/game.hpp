/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include "../base/net.hpp"

#include <thread>
#include <atomic>
#include <set>
#include <string>
#include <mutex>
#include <queue>
#include <stack>

#include "random.hpp"

namespace genie {

class MenuLobby;

extern void check_taunt(const std::string &str);

class MultiplayerCallback {
public:
	virtual ~MultiplayerCallback() {}
	virtual void chat(const TextMsg &msg) = 0;
	virtual void chat(user_id from, const std::string &str) = 0;
	virtual void join(JoinUser &usr) = 0;
	virtual void leave(user_id id) = 0;
	virtual void start(const StartMatch &settings) = 0;
};

namespace game {

class GameCallback;

}

class Multiplayer {
protected:
	Net net;
	std::string name;
	uint16_t port;
	std::thread t_worker;
	std::recursive_mutex mut; // lock for all following variables
	MultiplayerCallback &cb;
	game::GameCallback *gcb;
	bool invalidated;
public:
	user_id self;

	Multiplayer(MultiplayerCallback &cb, const std::string &name, uint16_t port);
	virtual ~Multiplayer() {}

	void dispose();

	virtual void eventloop() = 0;

	virtual bool chat(const std::string &str, bool send=true) = 0;
};

class Slave final {
	sockfd fd;
public:
	user_id id; /**< unique identifier (is equal to server's modification counter at creation) */
	player_id pid; /**< virtual player unique identifier (also used to detect modification changes) */
	std::string name;

	Slave(sockfd fd);
	Slave(sockfd fd, user_id id);
	// serversocket only: for id == 0
	Slave(const std::string &name);

	friend bool operator<(const Slave &lhs, const Slave &rhs);
};

class MultiplayerHost final : public Multiplayer, protected ServerCallback {
	ServerSocket sock;
	std::set<Slave> slaves;
	user_id idmod;
	Ready expected_settings; /**< data that each client has to send that must match */
	unsigned ready_confirms; /**< pending ready messages from slaves */
	bool dedicated; /**< whether the server is running headless (i.e. without a GUI) */
public:
	MultiplayerHost(MultiplayerCallback &cb, const std::string &name, uint16_t port, bool dedicated=false);
	~MultiplayerHost() override;

private:
	Slave &slave(sockfd fd);
public:
	void eventloop() override;
	void incoming(pollev &ev) override;
	void removepeer(sockfd fd) override;
	void event_process(sockfd fd, Command &cmd) override;
	void shutdown() override;

	void dump();
	void set_gcb(game::GameCallback *gcb);

	bool try_start();

	bool chat(const std::string &str, bool send=true) override;
	// TODO enable user to customize map settings
	void prepare_match();
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
	std::map<user_id, Peer> peers;
public:
	MultiplayerClient(MultiplayerCallback &cb, const std::string &name, uint32_t addr, uint16_t port);
	~MultiplayerClient() override;

	void eventloop() override;
	void set_gcb(game::GameCallback *gcb, uint16_t slave_count, uint16_t prng_next);
	bool chat(const std::string &str, bool send=true) override;
};

namespace game {

enum class GameMode {
	single_player,
	multiplayer_host,
	multiplayer_client,
	editor
};

enum class GameState {
	init,
	running,
	paused,
	stopped,
	closed
};

enum class TileId {
	// FIXME support forest, water and borders
	FLAT1,
	FLAT2,
	FLAT3,
	FLAT4,
	FLAT5,
	FLAT6,
	FLAT7,
	FLAT8,
	FLAT9,
	HILL_CORNER_SOUTH_EAST1,
	HILL_CORNER_NORTH_WEST1,
	HILL_CORNER_SOUTH_WEST1,
	HILL_CORNER_NORTH_EAST1,
	HILL_SOUTH,
	HILL_WEST,
	HILL_EAST,
	HILL_NORTH,
	HILL_CORNER_SOUTH_EAST2,
	HILL_CORNER_NORTH_WEST2,
	HILL_CORNER_SOUTH_WEST2,
	HILL_CORNER_NORTH_EAST2,
	FLAT_CORNER_SOUTH_EAST,
	FLAT_CORNER_NORTH_WEST,
	FLAT_CORNER_SOUTH_WEST,
	FLAT_CORNER_NORTH_EAST,
	TILE_MAX
};

class Map final {
public:
	unsigned w, h;
	std::unique_ptr<uint8_t[]> tiles, heights; // y,x order

	Map(LCG &lcg, const StartMatch &settings);
};

enum class PlayerState {
	active = 0x01, /**< player is generating/processing events. */
	op     = 0x02, /**< player can directly control server. */
	human  = 0x04, /**< player is controlled by human slave. */
};

enum class PlayerCheat {
	no_fog             = 0x0001,
	reveal_map         = 0x0002,
	spy_resources      = 0x0004,
	instant_gather     = 0x0008,
	instant_build      = 0x0010,
	instant_train      = 0x0020,
	instant_convert    = 0x0040,
	instant_all        = instant_gather | instant_build | instant_train | instant_convert,
	no_res_costs       = 0x0080,
	teleport_units     = 0x0100,
	teleport_buildings = 0x0200,
	teleport_all       = teleport_units | teleport_buildings,
	god                = 0x03FF,
};

enum class HandicapType {
	res             = 0x0001,
	cost_units      = 0x0002,
	cost_buildings  = 0x0004,
	cost_techs      = 0x0008,
	gather          = 0x0010,
	build           = 0x0020,
	train_units     = 0x0040,
	train_buildings = 0x0080,
	train_techs     = 0x0100,
	train           = train_units | train_buildings | train_techs,
	convert         = 0x0200,
	repair_wonder   = 0x0400,
	repair          = 0x0800,
	wincounter      = 0x1000,
	trade           = 0x2000,
	all             = 0x3FFF,
};

struct PlayerHandicap final {
	HandicapType type;
	bool percent;
	union HandicapData {
		float fraction;
		int raw;
	} adjust;
};

/**
 * Virtual user, this makes it possible for multiple players to control the same user.
 */
class Player {
	player_id id; /**< unique identifier */
	PlayerState state;
	//std::multiset<PlayerHandicap> hc; /**< player handicap */
	PlayerCheat cheats; /**< active OP actions */
	unsigned ai; /**< determines the AI mode, zero if human player without handicap/assistance */
public:
	std::string name; /**< E.g. Ramses III, this does not have to be the slave name that controls it */

	Player(player_id id); // only used to lookup players in a set
	Player(player_id id, const std::string &name);

	friend bool operator<(const Player&, const Player&);
};

class GameCallback {
public:
	virtual ~GameCallback() {}

	virtual void new_player(const CreatePlayer&) = 0;
	virtual void assign_player(const AssignSlave&) = 0;
};

class Game : public GameCallback {
protected:
	Multiplayer *mp;
	MenuLobby *lobby;
	GameMode mode;
	GameState state;
	LCG lcg;
	StartMatch settings;
	std::set<Player> players;
	std::map<user_id, player_id> usertbl; /**< Lookuptable for user id using slave player id */
	std::recursive_mutex mut;
	unsigned ticks_per_second;
	double tick_interval;
	double tick_timer;
public:
	Map map;

	Game(GameMode mode, MenuLobby *lobby, Multiplayer *mp, const StartMatch &settings);
	~Game();

private:
	void tick(unsigned n=1);
public:
	void step(unsigned ms);
	void step(double sec);
	void chmode(GameMode mode);
};

}

}
