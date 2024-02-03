#include "../src/server.hpp"

#include <gtest/gtest.h>

namespace aoe {

TEST(Pkg, Init1_512) {
	unsigned exp_type = 1, exp_payload = 512;
	NetPkgHdr h(exp_type, exp_payload);
	if (!h.native_ordering)
		FAIL() << "should be native order";
	if (h.type != exp_type)
		FAIL() << "bad type, expected " << exp_type << ", got" << h.type;
	if (h.payload != exp_payload)
		FAIL() << "bad payload, expected " << exp_payload << ", got " << h.payload;
}

TEST(Pkg, NtohHton) {
	NetPkgHdr h(1, 512);
	h.ntoh();
	if (!h.native_ordering)
		FAIL() << "should be native order";
	h.hton();
	if (h.native_ordering)
		FAIL() << "should be network order";
}

TEST(Pkg, ProtocolVersion) {
	NetPkg pkg(0, 0);
	uint16_t exp_prot = 1024;
	pkg.set_protocol(exp_prot);
	pkg.hton();
	pkg.ntoh();
	if (exp_prot != pkg.protocol_version())
		FAIL() << "bad protocol version, expected 0x" << std::hex << exp_prot << ", got " << pkg.protocol_version() << std::dec;
}

}
