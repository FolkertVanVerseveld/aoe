#include "../../server.hpp"

#include <except.hpp>

namespace aoe {

std::string NetPkg::username() {
	read(NetPkgType::set_username, "40s");
	return str(0);
}

void NetPkg::set_username(const std::string &s) {
	PkgWriter out(*this, NetPkgType::set_username);
	write("40s", pkgargs{ s }, false);
}

}
