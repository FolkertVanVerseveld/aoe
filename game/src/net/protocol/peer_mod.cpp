#include "../../server.hpp"

#include <except.hpp>

namespace aoe {

static void refcheck(const IdPoolRef &ref) {
	if (ref == invalid_ref)
		throw std::runtime_error("invalid ref");
}

void NetPkg::set_incoming(IdPoolRef ref) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	clear();
	writef("2IH", ref.first, ref.second, NetPeerControlType::incoming);
}

void NetPkg::set_peer_ref(IdPoolRef ref) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	clear();
	writef("2IH", ref.first, ref.second, NetPeerControlType::set_peer_ref);
}

void NetPkg::set_dropped(IdPoolRef ref) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	clear();
	writef("2IH", ref.first, ref.second, NetPeerControlType::dropped);
}

void NetPkg::set_claim_player(IdPoolRef ref, uint16_t idx) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	clear();
	writef("2I2H", ref.first, ref.second, NetPeerControlType::set_player_idx, idx);
}

void NetPkg::set_ref_username(IdPoolRef ref, const std::string &s) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	clear();
	writef("2IH40s", ref.first, ref.second, NetPeerControlType::set_username, s.size(), s.c_str());
}

NetPeerControl NetPkg::get_peer_control() {
	ZoneScoped;
	unsigned pos = read(NetPkgType::peermod, "2IH");

	IdPoolRef ref{ u32(0), u32(1) };
	NetPeerControlType type = (NetPeerControlType)u16(2);

	switch (type) {
	case NetPeerControlType::set_username:
		pos += read("40s", args, pos);
		return NetPeerControl(ref, str(3));
	case NetPeerControlType::set_player_idx:
		pos += read("H", args, pos);
		return NetPeerControl(ref, type, u16(3));
	default:
		return NetPeerControl(ref, type);
	}
}

}
