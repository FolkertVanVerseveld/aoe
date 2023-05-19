#include "net.hpp"

#include <variant>
#include <vector>
#include <utility>
#include <cstdint>
#include <initializer_list>

#include <idpool.hpp>

namespace aoe {

class PkgWriter;

typedef std::variant<uint64_t, std::string> netarg;
typedef std::vector<netarg> netargs;
typedef std::initializer_list<netarg> pkgargs;

class NetPkg final {
public:
	NetPkgHdr hdr;
	std::vector<uint8_t> data;
	netargs args;

	static constexpr unsigned max_payload = tcp4_max_size - NetPkgHdr::size;

	friend PkgWriter;

	NetPkg() : hdr(0, 0, false), data(), args() {}
	NetPkg(uint16_t type, uint16_t payload) : hdr(type, payload), data(), args() {}
	/** munch as much data we need and check if valid. throws if invalid or not enough data. */
	NetPkg(std::deque<uint8_t> &q);

	void set_protocol(uint16_t version);
	uint16_t protocol_version();

	void set_chat_text(IdPoolRef, const std::string&);
	std::pair<IdPoolRef, std::string> chat_text();

	void set_username(const std::string&);
	std::string username();

	void set_start_game();
	void set_gameover(unsigned);
	unsigned get_gameover();

	void set_scn_vars(const ScenarioSettings&);
	ScenarioSettings get_scn_vars();

	void set_player_resize(size_t);
	void claim_player_setting(uint16_t); // client to server
	void set_cpu_player(uint16_t); // both directions
	void set_player_civ(uint16_t, uint16_t);
	void set_player_team(uint16_t, uint16_t);
	void set_player_name(uint16_t, const std::string&);
	void set_player_died(uint16_t); // server to client
	void set_player_score(uint16_t, const PlayerAchievements&); // server to client
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
	void entity_train(IdPoolRef, EntityType);
	NetEntityMod get_entity_mod();

	void particle_spawn(const Particle &p);
	Particle get_particle();

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

	unsigned read(const std::string &fmt, netargs &args, unsigned offset=0);
	unsigned write(const std::string &fmt, const netargs &args, bool append);

	int8_t i8(unsigned pos) const;
	uint8_t u8(unsigned pos) const;
	uint16_t u16(unsigned pos) const;
	int32_t i32(unsigned pos) const;
	uint32_t u32(unsigned pos) const;
};

}
