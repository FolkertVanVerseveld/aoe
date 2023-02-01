#include "../server.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

namespace aoe {

static void refcheck(const IdPoolRef &ref) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");
}

void NetPkgHdr::ntoh() {
	if (native_ordering)
		return;

	type = ntohs(type);
	payload = ntohs(payload);

	native_ordering = true;
}

void NetPkgHdr::hton() {
	if (!native_ordering)
		return;

	type = htons(type);
	payload = htons(payload);

	native_ordering = false;
}

void NetPkg::need_payload(size_t n) {
	if (data.size() < n)
		throw std::runtime_error("corrupt data");
}

void NetPkg::ntoh() {
	if (hdr.native_ordering)
		return;

	switch ((NetPkgType)ntohs(hdr.type)) {
		case NetPkgType::set_protocol:
		case NetPkgType::set_username: {
			need_payload(1 * sizeof(uint16_t));

			uint16_t *dw = (uint16_t*)data.data();

			dw[0] = ntohs(dw[0]);
			break;
		}
		case NetPkgType::chat_text: {
			need_payload(2 * sizeof(uint32_t) + 1 * sizeof(uint16_t));

			uint32_t *dd = (uint32_t*)data.data();

			dd[0] = ntohl(dd[0]);
			dd[1] = ntohl(dd[1]);

			uint16_t *dw = (uint16_t*)&dd[2];

			dw[0] = ntohs(dw[0]);
			need_payload(2 * sizeof(uint32_t) + 1 * sizeof(uint16_t) + dw[0]);
			break;
		}
		case NetPkgType::playermod: {
			need_payload(NetPlayerControl::resize_size);

			uint16_t *dw = (uint16_t*)data.data();

			dw[0] = ntohs(dw[0]);
			NetPlayerControlType type = (NetPlayerControlType)dw[0];

			dw[1] = ntohs(dw[1]);

			switch (type) {
			case NetPlayerControlType::set_player_name:
			case NetPlayerControlType::set_civ:
			case NetPlayerControlType::set_team:
				dw[2] = ntohs(dw[2]);
				break;
			}

			break;
		}
		case NetPkgType::peermod: {
			size_t minsize = 2 * sizeof(uint32_t) + sizeof(uint16_t);
			need_payload(minsize);

			uint32_t *dd = (uint32_t*)data.data();

			dd[0] = ntohl(dd[0]);
			dd[1] = ntohl(dd[1]);

			uint16_t *dw = (uint16_t*)&dd[2];

			dw[0] = ntohs(dw[0]);
			NetPeerControlType type = (NetPeerControlType)dw[0];

			switch (type) {
			case NetPeerControlType::set_username:
				need_payload(minsize + sizeof(uint16_t));
				dw[1] = ntohs(dw[1]);
				need_payload(minsize + sizeof(uint16_t) + dw[1]);
				break;
			default:
				break;
			}
			break;
		}
		case NetPkgType::set_scn_vars: {
			need_payload(10 * sizeof(uint32_t) + 1);

			uint32_t *dd = (uint32_t*)data.data();

			for (unsigned i = 0; i < 10; ++i)
				dd[i] = ntohl(dd[i]);

			break;
		}
		case NetPkgType::start_game:
		case NetPkgType::gameover:
			// no payload
			break;
		case NetPkgType::terrainmod: {
			need_payload(NetTerrainMod::possize);
			uint16_t *dw = (uint16_t*)data.data();

			for (unsigned i = 0; i < 4; ++i)
				dw[i] = ntohs(dw[i]);

			size_t tsize = dw[2] * dw[3];
			need_payload(NetTerrainMod::possize + tsize * 2);

			break;
		}
		case NetPkgType::resmod: {
			need_payload(NetPkg::ressize);

			int32_t *dd = (int32_t*)data.data();

			for (unsigned i = 0; i < 4; ++i)
				dd[i] = ntohl(dd[i]);

			break;
		}
		case NetPkgType::entity_mod: {
			need_payload(NetEntityMod::minsize);
			uint16_t *dw = (uint16_t*)data.data();
			uint32_t *dd;

			dw[0] = ntohs(dw[0]); // type

			switch ((NetEntityControlType)dw[0]) {
			case NetEntityControlType::add:
			case NetEntityControlType::spawn:
				need_payload(NetEntityMod::addsize);
				dw[1] = ntohs(dw[1]); // e.type

				dd = (uint32_t*)&dw[2];

				// e.ref
				dd[0] = ntohl(dd[0]);
				dd[1] = ntohl(dd[1]);

				dd[2] = ntohl(dd[2]); // e.x
				dd[3] = ntohl(dd[3]); // e.y

				dw = (uint16_t*)&dd[4];
				dw[0] = ntohs(dw[0]); // e.color
				dw[1] = ntohs(dw[1]); // e.subimage
				break;
			case NetEntityControlType::kill:
				need_payload(NetEntityMod::killsize);
				dd = (uint32_t*)&dw[1];
				dd[0] = ntohl(dd[0]); dd[1] = ntohl(dd[1]);
				break;
			default:
				throw std::runtime_error("bad entity control type");
			}
			break;
		}
		default:
			throw std::runtime_error("bad type");
	}

	hdr.ntoh();
}

void NetPkg::hton() {
	if (!hdr.native_ordering)
		return;

	switch ((NetPkgType)hdr.type) {
		case NetPkgType::set_protocol:
		case NetPkgType::set_username: {
			uint16_t *dw = (uint16_t*)data.data();
			dw[0] = htons(dw[0]);
			break;
		}
		case NetPkgType::chat_text: {
			uint32_t *dd = (uint32_t*)data.data();

			dd[0] = htonl(dd[0]);
			dd[1] = htonl(dd[1]);

			uint16_t *dw = (uint16_t*)&dd[2];

			dw[0] = htons(dw[0]);

			break;
		}
		case NetPkgType::playermod: {
			uint16_t *dw = (uint16_t*)data.data();

			NetPlayerControlType type = (NetPlayerControlType)dw[0];

			dw[0] = htons(dw[0]);
			dw[1] = htons(dw[1]);

			switch (type) {
			case NetPlayerControlType::set_player_name:
			case NetPlayerControlType::set_civ:
			case NetPlayerControlType::set_team:
				dw[2] = htons(dw[2]);
				break;
			}

			break;
		}
		case NetPkgType::peermod: {
			uint32_t *dd = (uint32_t*)data.data();

			dd[0] = htonl(dd[0]);
			dd[1] = htonl(dd[1]);

			uint16_t *dw = (uint16_t*)&dd[2];

			NetPeerControlType type = (NetPeerControlType)dw[0];
			dw[0] = htons(dw[0]);

			switch (type) {
			case NetPeerControlType::set_username:
				dw[1] = htons(dw[1]);
				break;
			default:
				break;
			}
			break;
		}
		case NetPkgType::set_scn_vars: {
			uint32_t *dd = (uint32_t*)data.data();

			for (unsigned i = 0; i < 10; ++i)
				dd[i] = htonl(dd[i]);

			break;
		}
		case NetPkgType::start_game:
		case NetPkgType::gameover:
			// no payload
			break;
		case NetPkgType::terrainmod: {
			uint16_t *dw = (uint16_t*)data.data();

			for (unsigned i = 0; i < 4; ++i)
				dw[i] = htons(dw[i]);

			break;
		}
		case NetPkgType::resmod: {
			int32_t *dd = (int32_t*)data.data();

			for (unsigned i = 0; i < 4; ++i)
				dd[i] = htonl(dd[i]);

			break;
		}
		case NetPkgType::entity_mod: {
			uint16_t *dw = (uint16_t*)data.data();
			uint32_t *dd;

			NetEntityControlType type = (NetEntityControlType)dw[0];
			dw[0] = htons(dw[0]);

			switch (type) {
			case NetEntityControlType::add:
			case NetEntityControlType::spawn:
				dw[1] = htons(dw[1]); // e.type

				dd = (uint32_t*)&dw[2];

				dd[0] = htonl(dd[0]); dd[1] = htonl(dd[1]); // e.ref
				dd[2] = htonl(dd[2]); // e.x
				dd[3] = htonl(dd[3]); // e.y

				dw = (uint16_t*)&dd[4];
				dw[0] = htons(dw[0]); // e.color
				dw[1] = htons(dw[1]); // e.subimage
				break;
			case NetEntityControlType::kill:
				dd = (uint32_t*)&dw[1];
				dd[0] = htonl(dd[0]); dd[1] = htonl(dd[1]);
				break;
			default:
				throw std::runtime_error("bad entity control type");
			}
			break;
		}
		default:
			throw std::runtime_error("bad type");
	}

	hdr.hton();
}

void NetPkg::write(std::deque<uint8_t> &q) {
	hton();

	static_assert(NetPkgHdr::size == 4);

	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	data.v[0] = this->hdr.type;
	data.v[1] = this->hdr.payload;

	for (unsigned i = 0; i < NetPkgHdr::size; ++i)
		q.emplace_back(data.b[i]);

	for (unsigned i = 0; i < this->data.size(); ++i)
		q.emplace_back(this->data[i]);
}

void NetPkg::write(std::vector<uint8_t> &q) {
	hton();

	static_assert(NetPkgHdr::size == 4);

	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	data.v[0] = this->hdr.type;
	data.v[1] = this->hdr.payload;

	for (unsigned i = 0; i < NetPkgHdr::size; ++i)
		q.emplace_back(data.b[i]);

	for (unsigned i = 0; i < this->data.size(); ++i)
		q.emplace_back(this->data[i]);
}

NetPkg::NetPkg(std::deque<uint8_t> &q) : hdr(0, 0, false), data() {
	if (q.size() < NetPkgHdr::size)
		throw std::runtime_error("bad pkg hdr");

	// read pkg hdr
	union hdr {
		uint16_t v[2];
		uint8_t b[4];
	} data;

	static_assert(sizeof(data) == NetPkgHdr::size);

	data.b[0] = q[0];
	data.b[1] = q[1];
	data.b[2] = q[2];
	data.b[3] = q[3];

	this->hdr = NetPkgHdr(data.v[0], data.v[1], false);
	unsigned need = ntohs(this->hdr.payload);

	if (q.size() < need + NetPkgHdr::size)
		throw std::runtime_error("missing pkg data");

	// read data
	this->data.resize(need);

	for (unsigned i = 0; i < need; ++i)
		this->data[i] = q[i + NetPkgHdr::size];

	// convert to native ordering
	ntoh();

	// all good, remove from q
	for (unsigned i = 0; i < need + NetPkgHdr::size; ++i)
		q.pop_front();
}

void NetPkg::set_hdr(NetPkgType type) {
	assert(data.size() <= max_payload);

	hdr.native_ordering = true;
	hdr.type = (unsigned)type;

	if (data.size() > NetPkg::max_payload)
		throw std::runtime_error("payload overflow");

	hdr.payload = (uint16_t)data.size();
}

void NetPkg::set_protocol(uint16_t version) {
	data.resize(2);

	uint16_t *dw = (uint16_t*)data.data();
	*dw = version;

	set_hdr(NetPkgType::set_protocol);
}

uint16_t NetPkg::protocol_version() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::set_protocol || data.size() != 2)
		throw std::runtime_error("not a protocol network packet");

	const uint16_t *dw = (const uint16_t*)data.data();
	return *dw;
}

void NetPkg::set_chat_text(IdPoolRef ref, const std::string &s) {
	assert(s.size() <= max_payload - 2 * sizeof(uint32_t) - 1 * sizeof(uint16_t));

	size_t n = s.size();
	data.resize(2 * sizeof(uint32_t) + 1 * sizeof(uint16_t) + n);

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = ref.first;
	dd[1] = ref.second;

	uint16_t *dw = (uint16_t*)&dd[2];

	dw[0] = (uint16_t)n;

	memcpy(&dw[1], s.data(), n);

	set_hdr(NetPkgType::chat_text);
}

std::pair<IdPoolRef, std::string> NetPkg::chat_text() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::chat_text)
		throw std::runtime_error("not a chat text packet");

	const uint32_t *dd = (const uint32_t*)data.data();

	IdPoolRef ref{ dd[0], dd[1] };

	const uint16_t *dw = (const uint16_t*)&dd[2];

	uint16_t n = dw[0];

	std::string s(n, ' ');
	memcpy(s.data(), &dw[1], n);

	return std::make_pair(ref, s);
}

std::string NetPkg::username() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::set_username)
		throw std::runtime_error("not a username packet");

	const uint16_t *dw = (const uint16_t*)data.data();

	uint16_t n = dw[0];

	std::string s(n, ' ');
	memcpy(s.data(), &dw[1], n);

	return s;
}

void NetPkg::set_username(const std::string &s) {
	size_t n = s.size();
	data.resize(2u + n);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)n;
	memcpy(&dw[1], s.data(), n);

	set_hdr(NetPkgType::set_username);
}

void NetPkg::set_start_game() {
	data.clear();
	set_hdr(NetPkgType::start_game);
}

void NetPkg::set_gameover() {
	data.clear();
	set_hdr(NetPkgType::gameover);
}

void NetPkg::set_scn_vars(const ScenarioSettings &scn) {
	data.resize(10 * sizeof(uint32_t));

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = scn.width;
	dd[1] = scn.height;
	dd[2] = scn.popcap;
	dd[3] = scn.age;
	dd[4] = scn.seed;
	dd[5] = scn.villagers;

	dd[6] = scn.res.food;
	dd[7] = scn.res.wood;
	dd[8] = scn.res.gold;
	dd[9] = scn.res.stone;

	uint8_t flags = 0;

	if (scn.fixed_start     ) flags |= 1 << 0;
	if (scn.explored        ) flags |= 1 << 1;
	if (scn.all_technologies) flags |= 1 << 2;
	if (scn.cheating        ) flags |= 1 << 3;
	if (scn.square          ) flags |= 1 << 4;
	if (scn.wrap            ) flags |= 1 << 5;
	if (scn.restricted      ) flags |= 1 << 6;

	data.emplace_back(flags);

	set_hdr(NetPkgType::set_scn_vars);
}

ScenarioSettings NetPkg::get_scn_vars() {
	ntoh();

	size_t size = 10 * sizeof(uint32_t) + 1;

	if ((NetPkgType)hdr.type != NetPkgType::set_scn_vars || data.size() != size)
		throw std::runtime_error("not a scenario settings variables packet");

	const uint32_t *dd = (const uint32_t*)data.data();

	ScenarioSettings scn;

	scn.width     = dd[0];
	scn.height    = dd[1];
	scn.popcap    = dd[2];
	scn.age       = dd[3];
	scn.seed      = dd[4];
	scn.villagers = dd[5];

	scn.res.food  = dd[6];
	scn.res.wood  = dd[7];
	scn.res.gold  = dd[8];
	scn.res.stone = dd[9];

	const uint8_t *db = (const uint8_t*)&dd[10];

	uint8_t flags = db[0];

	scn.fixed_start      = !!(flags & (1 << 0));
	scn.explored         = !!(flags & (1 << 1));
	scn.all_technologies = !!(flags & (1 << 2));
	scn.cheating         = !!(flags & (1 << 3));
	scn.square           = !!(flags & (1 << 4));
	scn.wrap             = !!(flags & (1 << 5));
	scn.restricted       = !!(flags & (1 << 6));

	return scn;
}

void NetPkg::set_player_resize(size_t size) {
	if (size > UINT16_MAX)
		throw std::runtime_error("overflow player resize");

	data.resize(NetPlayerControl::resize_size);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)NetPlayerControlType::resize;
	dw[1] = (uint16_t)size;

	set_hdr(NetPkgType::playermod);
}

void NetPkg::claim_player_setting(uint16_t idx) {
	data.resize(NetPlayerControl::resize_size);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)NetPlayerControlType::set_ref;
	dw[1] = idx;

	set_hdr(NetPkgType::playermod);
}

void NetPkg::set_player_died(uint16_t idx) {
	data.resize(NetPlayerControl::resize_size);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)NetPlayerControlType::died;
	dw[1] = idx;

	set_hdr(NetPkgType::playermod);
}

void NetPkg::claim_cpu_setting(uint16_t idx) {
	data.resize(NetPlayerControl::resize_size);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)NetPlayerControlType::set_cpu_ref;
	dw[1] = (uint16_t)idx;

	set_hdr(NetPkgType::playermod);
}

NetPlayerControl NetPkg::get_player_control() {
	ntoh();

	// TODO check data size
	if ((NetPkgType)hdr.type != NetPkgType::playermod)
		throw std::runtime_error("not a player control packet");

	const uint16_t *dw = (const uint16_t*)data.data();

	NetPlayerControlType type = (NetPlayerControlType)dw[0];

	switch (type) {
		case NetPlayerControlType::resize:
		case NetPlayerControlType::erase:
		case NetPlayerControlType::died:
		case NetPlayerControlType::set_ref:
		case NetPlayerControlType::set_cpu_ref:
			return NetPlayerControl(type, dw[1]);
		case NetPlayerControlType::set_player_name: {
			uint16_t idx = dw[1], n = dw[2];

			std::string name(n, ' ');
			memcpy(name.data(), &dw[3], n);

			return NetPlayerControl(type, idx, name);
		}
		case NetPlayerControlType::set_civ:
		case NetPlayerControlType::set_team: {
			uint16_t idx = dw[1], pos = dw[2];
			return NetPlayerControl(type, idx, pos);
		}
		default:
			throw std::runtime_error("bad player control type");
	}
}

void NetPkg::set_incoming(IdPoolRef ref) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");

	data.resize(2 * sizeof(uint32_t) + sizeof(uint16_t));

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = ref.first;
	dd[1] = ref.second;

	uint16_t *dw = (uint16_t*)&dd[2];

	dw[0] = (uint16_t)(unsigned)NetPeerControlType::incoming;

	set_hdr(NetPkgType::peermod);
}

void NetPkg::set_dropped(IdPoolRef ref) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");

	data.resize(2 * sizeof(uint32_t) + sizeof(uint16_t));

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = ref.first;
	dd[1] = ref.second;

	uint16_t *dw = (uint16_t*)&dd[2];

	dw[0] = (uint16_t)(unsigned)NetPeerControlType::dropped;

	set_hdr(NetPkgType::peermod);
}

void NetPkg::set_claim_player(IdPoolRef ref, uint16_t idx) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");

	data.resize(2 * sizeof(uint32_t) + 2 * sizeof(uint16_t));

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = ref.first;
	dd[1] = ref.second;

	uint16_t *dw = (uint16_t*)&dd[2];

	dw[0] = (uint16_t)(unsigned)NetPeerControlType::set_player_idx;
	dw[1] = idx;

	set_hdr(NetPkgType::peermod);
}

void NetPkg::set_cpu_player(uint16_t idx) {
	data.resize(NetPlayerControl::resize_size);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)NetPlayerControlType::set_cpu_ref;
	dw[1] = (uint16_t)idx;

	set_hdr(NetPkgType::playermod);
}

void NetPkg::playermod2(NetPlayerControlType type, uint16_t idx, uint16_t pos) {
	data.resize(NetPlayerControl::set_pos_size);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)type;
	dw[1] = idx;
	dw[2] = pos;

	set_hdr(NetPkgType::playermod);
}

void NetPkg::set_player_civ(uint16_t idx, uint16_t civ) {
	playermod2(NetPlayerControlType::set_civ, idx, civ);
}

void NetPkg::set_player_team(uint16_t idx, uint16_t team) {
	playermod2(NetPlayerControlType::set_team, idx, team);
}

void NetPkg::set_player_name(uint16_t idx, const std::string &s) {
	size_t minsize = NetPlayerControl::resize_size + sizeof(uint16_t);

	data.resize(minsize + s.size());

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)NetPlayerControlType::set_player_name;
	dw[1] = idx;
	dw[2] = (uint16_t)s.size();

	memcpy(&dw[3], s.data(), s.size());

	set_hdr(NetPkgType::playermod);
}

void NetPkg::set_ref_username(IdPoolRef ref, const std::string &s) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");

	size_t minsize = 2 * sizeof(uint32_t) + 2 * sizeof(uint16_t);

	data.resize(minsize + s.size());

	uint32_t *dd = (uint32_t*)data.data();

	dd[0] = ref.first;
	dd[1] = ref.second;

	uint16_t *dw = (uint16_t*)&dd[2];

	dw[0] = (uint16_t)(unsigned)NetPeerControlType::set_username;
	dw[1] = (uint16_t)s.size();

	memcpy(&dw[2], s.data(), s.size());

	set_hdr(NetPkgType::peermod);
}

NetPeerControl NetPkg::get_peer_control() {
	ZoneScoped;
	ntoh();

	// TODO remove data.size() check
	if ((NetPkgType)hdr.type != NetPkgType::peermod)
		throw std::runtime_error("not a peer control packet");

	const uint32_t *dd = (const uint32_t*)data.data();
	const uint16_t *dw = (const uint16_t*)&dd[2];

	NetPeerControlType type = (NetPeerControlType)dw[0];
	IdPoolRef ref{ dd[0], dd[1] };

	switch (type) {
		case NetPeerControlType::set_username: {
			uint16_t n = dw[1];

			std::string s(n, ' ');
			memcpy(s.data(), &dw[2], n);

			return NetPeerControl(ref, s);
		}
		case NetPeerControlType::set_player_idx:
			return NetPeerControl(ref, type, dw[1]);
		default:
			return NetPeerControl(ref, (NetPeerControlType)dw[0]);
	}
}

void NetPkg::set_entity_add(const Entity &e) {
	EntityView ev(e.ref, e.type, e.color, e.x, e.y);
	set_entity_add(ev);
}

void NetPkg::set_entity_add(const EntityView &e) {
	entity_add(e, NetEntityControlType::add);
}

void NetPkg::set_entity_spawn(const Entity &e) {
	EntityView ev(e.ref, e.type, e.color, e.x, e.y);
	set_entity_spawn(ev);
}

void NetPkg::set_entity_spawn(const EntityView &e) {
	entity_add(e, NetEntityControlType::spawn);
}

void NetPkg::entity_add(const EntityView &e, NetEntityControlType type) {
	static_assert(sizeof(float) <= sizeof(uint32_t));

	refcheck(e.ref);

	data.resize(NetEntityMod::addsize);

	/*
	layout:
	b # sz  name
	2 1 u16 type
	2 1 u16 e.type
	8 2 u32 e.ref
	4 1 u32 e.x
	4 1 u32 e.y
	2 1 u16 e.color
	2 1 u16 e.subimage
	*/

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)type;
	dw[1] = (uint16_t)e.type;

	uint32_t *dd = (uint32_t*)&dw[2];

	dd[0] = e.ref.first;
	dd[1] = e.ref.second;

	dd[2] = (uint32_t)e.x;
	dd[3] = (uint32_t)e.y;

	dw = (uint16_t*)&dd[4];
	dw[0] = e.color;
	dw[1] = e.subimage;

	set_hdr(NetPkgType::entity_mod);
}

void NetPkg::set_entity_kill(IdPoolRef ref) {
	static_assert(sizeof(RefCounter) <= sizeof(uint32_t));

	refcheck(ref);

	// TODO byte misalignment. this will cause bus errors on archs that don't support unaligned fetching
	data.resize(NetEntityMod::killsize);

	uint16_t *dw = (uint16_t*)data.data();
	uint32_t *dd;

	dw[0] = (uint16_t)NetEntityControlType::kill;

	dd = (uint32_t*)&dw[1];
	dd[0] = ref.first; dd[1] = ref.second;

	set_hdr(NetPkgType::entity_mod);
}

NetEntityMod NetPkg::get_entity_mod() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::entity_mod)
		throw std::runtime_error("not an entity control packet");

	const uint16_t *dw = (const uint16_t*)data.data();
	const uint32_t *dd;

	NetEntityControlType type = (NetEntityControlType)dw[0];

	switch (type) {
	case NetEntityControlType::add:
	case NetEntityControlType::spawn: {
		EntityView ev;

		ev.type = (EntityType)dw[1];

		dd = (const uint32_t*)&dw[2];
		ev.ref.first = dd[0]; ev.ref.second = dd[1];
		ev.x = dd[2]; ev.y = dd[3];

		dw = (const uint16_t*)&dd[4];
		ev.color = dw[0];

		return NetEntityMod(ev);
	}
	case NetEntityControlType::kill: {
		IdPoolRef ref;

		dd = (uint32_t*)&dw[1];

		ref.first = dd[0]; ref.second = dd[1];

		return NetEntityMod(ref);
	}
	default:
		throw std::runtime_error("unknown entity control packet");
	}
}

void NetPkg::set_terrain_mod(const NetTerrainMod &tm) {
	ZoneScoped;
	assert(tm.tiles.size() == tm.hmap.size());
	size_t tsize = tm.w * tm.h * 2;

	if (tsize > max_payload - NetTerrainMod::possize)
		throw std::runtime_error("terrain mod too big");

	data.resize(NetTerrainMod::possize + tsize);

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = tm.x; dw[1] = tm.y;
	dw[2] = tm.w; dw[3] = tm.h;

	uint8_t *db = (uint8_t*)&dw[4];

	memcpy(db, tm.tiles.data(), tm.tiles.size());
	memcpy(db + tm.tiles.size(), tm.hmap.data(), tm.hmap.size());

	set_hdr(NetPkgType::terrainmod);
}

NetTerrainMod NetPkg::get_terrain_mod() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::terrainmod || data.size() < NetTerrainMod::possize)
		throw std::runtime_error("not a terrain control packet");

	const uint16_t *dw = (const uint16_t*)data.data();

	NetTerrainMod tm;

	tm.x = dw[0]; tm.y = dw[1];
	tm.w = dw[2]; tm.h = dw[3];

	const uint8_t *db = (const uint8_t*)&dw[4];

	// TODO check packet size
	size_t size = tm.w * tm.h;

	tm.tiles.resize(size);
	tm.hmap.resize(size);

	memcpy(tm.tiles.data(), db, size);
	memcpy(tm.hmap.data(), db + size, size);

	return tm;
}

void NetPkg::set_resources(const Resources &res) {
	ZoneScoped;

	data.resize(NetPkg::ressize);

	int32_t *dd = (int32_t*)data.data();

	dd[0] = res.food;
	dd[1] = res.wood;
	dd[2] = res.gold;
	dd[3] = res.stone;

	set_hdr(NetPkgType::resmod);
}

Resources NetPkg::get_resources() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::resmod || data.size() != NetPkg::ressize)
		throw std::runtime_error("not a resources control packet");

	const int32_t *dd = (const int32_t*)data.data();

	Resources res;

	res.food = dd[0];
	res.wood = dd[1];
	res.gold = dd[2];
	res.stone = dd[3];

	return res;
}

NetPkgType NetPkg::type() {
	ntoh();
	return (NetPkgType)hdr.type;
}

void Client::send(NetPkg &pkg) {
	pkg.hton();

	// prepare header
	uint16_t v[2];
	v[0] = pkg.hdr.type;
	v[1] = pkg.hdr.payload;

	send(v, 2);
	send(pkg.data.data(), (int)pkg.data.size());
}

NetPkg Client::recv() {
	NetPkg pkg;
	unsigned payload;

	// retrieve header
	uint16_t dw[2];
	recv(dw, 2);

	// parse header
	pkg.hdr.type = dw[0];
	pkg.hdr.payload = dw[1];

	// reserve payload
	payload = ntohs(pkg.hdr.payload);
	pkg.data.resize(payload);

	// retrieve payload
	recv(pkg.data.data(), payload);

	pkg.ntoh();

	return pkg;
}

}
