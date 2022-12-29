#include "server.hpp"

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
			need_payload(2 * sizeof(uint16_t));

			uint16_t *dw = (uint16_t*)data.data();

			dw[0] = ntohs(dw[0]);
			dw[1] = ntohs(dw[1]);
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
			// no payload
			break;
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

			dw[0] = htons(dw[0]);
			dw[1] = htons(dw[1]);
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
			// no payload
			break;
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

	if ((NetPkgType)hdr.type != NetPkgType::chat_text || data.size() > max_payload - 2 * sizeof(uint32_t) - 1 * sizeof(uint16_t))
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

	if ((NetPkgType)hdr.type != NetPkgType::set_username || data.size() > max_payload - 2)
		throw std::runtime_error("not a username packet");

	const uint16_t *dw = (const uint16_t*)data.data();

	uint16_t n = dw[0];

	std::string s(n, ' ');
	memcpy(s.data(), &dw[1], n);

	return s;
}

void NetPkg::set_username(const std::string &s) {
	assert(s.size() <= max_payload - 2);

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

void NetPkg::set_scn_vars(const ScenarioSettings &scn) {
	data.clear();

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

	data.resize(2 * sizeof(uint16_t));

	uint16_t *dw = (uint16_t*)data.data();

	dw[0] = (uint16_t)(unsigned)NetPlayerControlType::resize;
	dw[1] = (uint16_t)size;

	set_hdr(NetPkgType::playermod);
}

NetPlayerControl NetPkg::get_player_control() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::playermod || data.size() != 2 * sizeof(uint16_t))
		throw std::runtime_error("not a player control packet");

	const uint16_t *dw = (const uint16_t*)data.data();

	return NetPlayerControl((NetPlayerControlType)dw[0], dw[1]);
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

void NetPkg::set_ref_username(IdPoolRef ref, const std::string &s) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");

	size_t minsize = 2 * sizeof(uint32_t) + 2 * sizeof(uint16_t);

	if (minsize + s.size() > max_payload)
		throw std::runtime_error("username overflow");

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
	ntoh();

	// TODO remove data.size() check
	if ((NetPkgType)hdr.type != NetPkgType::peermod || data.size() < 2 * sizeof(uint32_t) + sizeof(uint16_t))
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
		default:
			return NetPeerControl(ref, (NetPeerControlType)dw[0]);
	}
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
