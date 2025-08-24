#include "../../server.hpp"

namespace aoe {

void NetPkg::set_ready(bool v) {
	PkgWriter out(*this, NetPkgType::client_info);
	clear();
	writef("B", v);
}

NetClientInfoControl NetPkg::get_client_info() {
	read(NetPkgType::client_info, "B");
	return NetClientInfoControl(u8(0));
}

}
