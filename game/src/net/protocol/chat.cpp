#include "../../server.hpp"

#include <except.hpp>

namespace aoe {

void NetPkg::set_chat_text(IdPoolRef ref, const std::string &s) {
	PkgWriter out(*this, NetPkgType::chat_text);
	clear();
	writef("2I80s", ref.first, ref.second, s.size(), s.c_str());
}

std::pair<IdPoolRef, std::string> NetPkg::chat_text() {
	read(NetPkgType::chat_text, "2I80s");
	IdPoolRef ref{ u32(0), u32(1) };
	return std::make_pair(ref, str(2));
}

}
