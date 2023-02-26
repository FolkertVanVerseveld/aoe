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
	set_cpu_ref,
	set_player_name,
	set_civ,
	set_team,
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
	std::variant<std::nullopt_t, IdPoolRef, uint16_t, std::pair<uint16_t, std::string>, std::pair<uint16_t, uint16_t>> data;

	static constexpr unsigned resize_size = 2 * sizeof(uint16_t);
	static constexpr unsigned set_pos_size = 3 * sizeof(uint16_t);

	NetPlayerControl() : type(NetPlayerControlType::resize), data(std::nullopt) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t arg) : type(type), data(arg) {}
	NetPlayerControl(NetPlayerControlType type, IdPoolRef ref) : type(type), data(ref) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t idx, const std::string &name) : type(type), data(std::make_pair(idx, name)) {}
	NetPlayerControl(NetPlayerControlType type, uint16_t idx, uint16_t pos) : type(type), data(std::make_pair(idx, pos)) {}
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
	// TODO byte misalignment. this will cause bus errors on archs that don't support unaligned fetching
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
	static constexpr size_t addsize = refsize + 2*4 + 2*2;

	NetParticleMod(const Particle &p) : data(p) {}
};

}
