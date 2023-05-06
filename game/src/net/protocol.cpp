#include "../server.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

namespace aoe {

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
		case NetPkgType::chat_text:
			break;
		case NetPkgType::playermod:
		case NetPkgType::gameover:
			// bytes are converted implicitly
			break;
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
		case NetPkgType::gamespeed_control:
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
			case NetEntityControlType::update:
				need_payload(NetEntityMod::addsize);
				dw[1] = ntohs(dw[1]); // e.type

				dd = (uint32_t*)&dw[2];

				// e.ref
				dd[0] = ntohl(dd[0]);
				dd[1] = ntohl(dd[1]);

				dd[2] = ntohl(dd[2]); // e.x
				dd[3] = ntohl(dd[3]); // e.y

				dw = (uint16_t*)&dd[4];
				dw[0] = ntohs(dw[0]); // e.angle
				dw[1] = ntohs(dw[1]); // e.color
				dw[2] = ntohs(dw[2]); // e.subimage

				// e.state
				// e.dx
				// e.dy
				// e.reserved

				dw[5] = ntohs(dw[5]); // e.stats.hp
				dw[6] = ntohs(dw[6]); // e.stats.maxhp
				break;
			case NetEntityControlType::kill:
				need_payload(NetEntityMod::killsize);
				dd = (uint32_t*)&dw[1];
				dd[0] = ntohl(dd[0]); dd[1] = ntohl(dd[1]);
				break;
			case NetEntityControlType::task:
				need_payload(NetEntityMod::tasksize);
				dw[1] = ntohs(dw[1]); // type

				dd = (uint32_t*)&dw[2];

				for (unsigned i = 0; i < 4; ++i) // e.ref, (e.ref2 OR e.x,e.y)
					dd[i] = ntohl(dd[i]);
				break;
			default:
				throw std::runtime_error("bad entity control type");
			}
			break;
		}
		case NetPkgType::cam_set: {
			need_payload(NetCamSet::size);

			int32_t *dd = (int32_t*)data.data();

			for (unsigned i = 0; i < 4; ++i)
				dd[i] = ntohl(dd[i]);

			break;
		}
		case NetPkgType::gameticks:
			need_payload(sizeof(uint16_t));
			break;
		case NetPkgType::particle_mod: {
			need_payload(NetParticleMod::addsize);

			uint32_t *dd = (uint32_t*)data.data();

			dd[0] = ntohl(dd[0]); dd[1] = ntohl(dd[1]); // p.ref

			uint16_t *dw = (uint16_t*)&dd[2];

			dw[0] = ntohs(dw[0]); // p.type
			dw[1] = ntohs(dw[1]); // p.subimage

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
		case NetPkgType::chat_text:
		case NetPkgType::playermod:
		case NetPkgType::gameover:
			// bytes are converted implicitly
			break;
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
		case NetPkgType::gamespeed_control:
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
			case NetEntityControlType::update:
				// size: addsize
				dw[1] = htons(dw[1]); // e.type

				dd = (uint32_t*)&dw[2];

				dd[0] = htonl(dd[0]); dd[1] = htonl(dd[1]); // e.ref
				dd[2] = htonl(dd[2]); // e.x
				dd[3] = htonl(dd[3]); // e.y

				dw = (uint16_t*)&dd[4];
				dw[0] = htons(dw[0]); // e.angle
				dw[1] = htons(dw[1]); // e.color
				dw[2] = htons(dw[2]); // e.subimage

				// e.state
				// e.dx
				// e.dy
				// [reserved]
				dw[5] = htons(dw[5]); // e.stats.hp
				dw[6] = htons(dw[6]); // e.stats.maxhp
				break;
			case NetEntityControlType::kill:
				dd = (uint32_t*)&dw[1];
				dd[0] = htonl(dd[0]); dd[1] = htonl(dd[1]);
				break;
			case NetEntityControlType::task:
				dw[1] = htons(dw[1]); // type

				dd = (uint32_t*)&dw[2];

				for (unsigned i = 0; i < 4; ++i) // e.ref, (e.ref2 OR e.x,e.y)
					dd[i] = htonl(dd[i]);
				break;
			default:
				throw std::runtime_error("bad entity control type");
			}
			break;
		}
		case NetPkgType::cam_set: {
			int32_t *dd = (int32_t*)data.data();

			for (unsigned i = 0; i < 4; ++i)
				dd[i] = htonl(dd[i]);

			break;
		}
		case NetPkgType::gameticks:
			break;
		case NetPkgType::particle_mod: {
			uint32_t *dd = (uint32_t*)data.data();

			dd[0] = htonl(dd[0]); dd[1] = htonl(dd[1]); // p.ref

			uint16_t *dw = (uint16_t*)&dd[2];

			dw[0] = htons(dw[0]); // p.type
			dw[1] = htons(dw[1]); // p.subimage

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
	PkgWriter out(*this, NetPkgType::chat_text);
	write("2I80s", { ref.first, ref.second, s }, false);
}

std::pair<IdPoolRef, std::string> NetPkg::chat_text() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::chat_text)
		throw std::runtime_error("not a chat text packet");

	std::vector<std::variant<uint64_t, std::string>> args;
	read("2I80s", args);

	IdPoolRef ref{ std::get<uint64_t>(args.at(0)), std::get<uint64_t>(args.at(1)) };
	return std::make_pair(ref, std::get<std::string>(args.at(2)));
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

void NetPkg::set_gameover(unsigned team) {
	PkgWriter out(*this, NetPkgType::gameover);
	write("H", { team }, false);
}

unsigned NetPkg::get_gameover() {
	std::vector<std::variant<uint64_t, std::string>> args;
	unsigned pos = 0;

	read("H", args, pos);

	return (unsigned)std::get<uint64_t>(args.at(0));
}

void NetPkg::cam_set(float x, float y, float w, float h) {
	data.resize(NetCamSet::size);

	int32_t *dd = (int32_t*)data.data();

	dd[0] = (int32_t)x; dd[1] = (int32_t)y;
	dd[2] = (int32_t)w; dd[3] = (int32_t)h;

	set_hdr(NetPkgType::cam_set);
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

NetCamSet NetPkg::get_cam_set() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::cam_set)
		throw std::runtime_error("not a camera set packet");

	NetCamSet s;

	const int32_t *dd = (const int32_t*)data.data();

	s.x = dd[0];
	s.y = dd[1];
	s.w = dd[2];
	s.h = dd[3];

	return s;
}

uint16_t NetPkg::get_gameticks() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::gameticks)
		throw std::runtime_error("not a gameticks packet");

	std::vector<std::variant<uint64_t, std::string>> args;
	read("H", args);

	return (uint16_t)std::get<uint64_t>(args.at(0));
}

void NetPkg::set_gameticks(unsigned n) {
	ZoneScoped;
	assert(n <= UINT16_MAX);

	PkgWriter out(*this, NetPkgType::gameticks);
	write("H", { n }, false);
}

NetGamespeedControl NetPkg::get_gamespeed() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::gamespeed_control)
		throw std::runtime_error("not a gamespeed control packet");

	const uint8_t *db = (const uint8_t*)data.data();
	NetGamespeedType type = (NetGamespeedType)db[0];

	if (db[0] > (uint8_t)NetGamespeedType::decrease)
		throw std::runtime_error("invalid gamespeed control type");

	return NetGamespeedControl(type);
}

void NetPkg::set_gamespeed(NetGamespeedType type) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::gamespeed_control, NetGamespeedControl::size);

	uint8_t *db = (uint8_t*)data.data();
	db[0] = (uint8_t)type;
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
