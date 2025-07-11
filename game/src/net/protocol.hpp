#pragma once

#include <cstddef>
#include <cstdint>

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
	entity_mod,
	gameover,
	cam_set,
	gameticks,
	particle_mod,
	gamespeed_control,
	client_info,
};

enum NetStartGameType {
	announce,
	now,
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

class NetCamSet final {
public:
	int32_t x, y, w, h;

	static constexpr size_t size = 4 * sizeof(int32_t);

	NetCamSet() : x(0), y(0), w(1), h(1) {}
	NetCamSet(int32_t x, int32_t y, int32_t w, int32_t h) : x(x), y(y), w(w), h(h) {}
};

enum class NetPlayerControlType {
	resize,
	erase,
	died,
	set_ref,
	set_player_name,
	set_civ,
	set_team,
	set_score,
};

enum class NetPeerControlType {
	incoming,
	dropped,
	set_username,
	set_player_idx,
	set_peer_ref,
};

struct NetPlayerScore final {
	uint32_t military;
	int32_t economy;
	uint32_t religion;
	uint32_t technology;
	int64_t score;
	uint16_t wonders;
	uint16_t technologies;
	uint16_t playerid;
	uint8_t age;
	bool alive;
	bool most_technologies;
	bool bronze_first;
	bool iron_first;

	static constexpr unsigned size = 4 * 4 + 8 + 3 * 2 + 1 + 1;
};

class NetPlayerControl final {
public:
	NetPlayerControlType type;
	std::variant<std::nullopt_t, IdPoolRef, uint16_t, std::pair<uint16_t, std::string>, std::pair<uint16_t, uint16_t>, NetPlayerScore> data;

	static constexpr unsigned resize_size = 2 * sizeof(uint16_t);
	static constexpr unsigned set_pos_size = 3 * sizeof(uint16_t);
	static constexpr unsigned set_score_size = 2 + NetPlayerScore::size;

	NetPlayerControl() : type(NetPlayerControlType::resize), data(std::nullopt) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t arg) : type(type), data(arg) {}
	NetPlayerControl(NetPlayerControlType type, IdPoolRef ref) : type(type), data(ref) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t idx, const std::string &name) : type(type), data(std::make_pair(idx, name)) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t idx, uint16_t pos) : type(type), data(std::make_pair(idx, pos)) {}
	NetPlayerControl(const NetPlayerScore &s) : type(NetPlayerControlType::set_score), data(s) {}
};

class NetPeerControl final {
public:
	IdPoolRef ref;
	NetPeerControlType type;
	std::variant<std::nullopt_t, std::string, uint16_t> data;

	NetPeerControl(IdPoolRef ref, NetPeerControlType type) : ref(ref), type(type), data(std::nullopt) {}
	NetPeerControl(IdPoolRef ref, const std::string &name) : ref(ref), type(NetPeerControlType::set_username), data(name) {}
	NetPeerControl(IdPoolRef ref, NetPeerControlType type, uint16_t idx) : ref(ref), type(type), data(idx) {}

	const uint16_t u16() const { return std::get<uint16_t>(data); }
};

class NetClientInfoControl final {
public:
	uint8_t v;

	NetClientInfoControl(uint8_t v) : v(v) {}
};

class NetTerrainMod final {
public:
	uint16_t x, y, w, h;
	std::vector<uint16_t> tiles;
	std::vector<uint8_t> hmap;

	static constexpr size_t possize = 4 * sizeof(uint16_t);

	NetTerrainMod() : x(0), y(0), w(0), h(0), tiles(), hmap() {}
};

enum class NetEntityControlType {
	add,
	spawn,
	update,
	kill,
	task,
};

static constexpr size_t refsize = 2 * sizeof(uint32_t);

class NetEntityMod final {
public:
	NetEntityControlType type;
	std::variant<std::nullopt_t, IdPoolRef, EntityView, EntityTask> data;

	static constexpr size_t minsize = 2;
	static constexpr size_t killsize = minsize + refsize;
	/*
	2 minsize
	2 type
	2*4 ref
	4 x
	4 y
	2 angle
	2 color
	2 subimage
	1 state
	1 dx
	1 dy
	1 attack
	2 hp
	2 maxhp
	*/
	static constexpr size_t addsize = minsize + 2 + refsize + 2*4 + 3*2 + 4*1 + 2*2;
	/*
	2 minsize
	2 type
	2*4 ref
	2*4 ref OR x,y
	*/
	static constexpr size_t tasksize = minsize + 2 + refsize + 2*4;

	NetEntityMod(IdPoolRef ref) : type(NetEntityControlType::kill), data(ref) {}
	NetEntityMod(const EntityView &e, NetEntityControlType t) : type(t), data(e) {}
	NetEntityMod(const EntityTask &t) : type(NetEntityControlType::task), data(t) {}
};

class NetParticleMod final {
public:
	Particle data;

	/*
	2*4 ref
	4 x
	4 y
	2 type
	2 subimage
	*/
	NetParticleMod(const Particle &p) : data(p) {}
};

enum class NetGamespeedType {
	// NOTE pause/unpause are reserved for server to client
	pause,
	unpause,

	toggle_pause, // NOTE this type is only for client to server
	increase,
	decrease,
	// TODO create SET
};

class NetGamespeedControl final {
public:
	NetGamespeedType type;
	uint8_t speed; // only used by server to announce new gamespeed

	static constexpr size_t size = 2;

	NetGamespeedControl(NetGamespeedType t, uint8_t speed) : type(t), speed(speed) {}
};

}
