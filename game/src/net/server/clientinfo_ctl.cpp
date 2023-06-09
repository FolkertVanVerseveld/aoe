#include "../../server.hpp"

#include <cstdio>

namespace aoe {

bool Server::process_clientinfo(const Peer &p, NetPkg &pkg) {
	NetClientInfoControl ctl(pkg.get_client_info());
	uint8_t v = ctl.v;

	lock lk(m_peers);
	ClientInfo &ci = get_ci(peer2ref(p));

	// TODO add more flags??
	if (v) ci.flags |= (unsigned)ClientInfoFlags::ready;
	else ci.flags &= ~(unsigned)ClientInfoFlags::ready;

	return true;
}

}
