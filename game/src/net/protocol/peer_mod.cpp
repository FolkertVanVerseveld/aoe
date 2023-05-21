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

	write("2IH", pkgargs{
		ref.first, ref.second, (unsigned)NetPeerControlType::incoming
	}, false);
}

void NetPkg::set_dropped(IdPoolRef ref) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	write("2IH", pkgargs{
		ref.first, ref.second, (unsigned)NetPeerControlType::dropped
	}, false);
}

void NetPkg::set_claim_player(IdPoolRef ref, uint16_t idx) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	write("2I2H", pkgargs{
		ref.first, ref.second, (unsigned)NetPeerControlType::set_player_idx, idx
	}, false);
}

void NetPkg::set_ref_username(IdPoolRef ref, const std::string &s) {
	refcheck(ref);
	PkgWriter out(*this, NetPkgType::peermod);

	write("2IH40s", pkgargs{
		ref.first, ref.second,
		(unsigned)NetPeerControlType::set_username, s
	}, false);
}

}
