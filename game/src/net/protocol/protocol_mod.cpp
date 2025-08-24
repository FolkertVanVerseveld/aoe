#include "../../server.hpp"

#include <except.hpp>

namespace aoe {

void NetPkg::set_protocol(uint16_t version) {
	PkgWriter out(*this, NetPkgType::set_protocol);
	clear();
	writef("H", version);
}

uint16_t NetPkg::protocol_version() {
	read(NetPkgType::set_protocol, "H");
	return u16(0);
}

}
