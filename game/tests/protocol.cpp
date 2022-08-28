#include "../src/server.hpp"

namespace aoe {

static void pkg_init_1_512() {
	unsigned exp_type = 1, exp_payload = 512;
	NetPkgHdr h(exp_type, exp_payload);
	if (!h.native_ordering)
		fprintf(stderr, "%s: should be native order\n", __func__);
	if (h.type != exp_type)
		fprintf(stderr, "%s: bad type, expected %u, got %u\n", __func__, exp_type, h.type);
	if (h.payload != exp_payload)
		fprintf(stderr, "%s: bad payload, expected %u, got %u\n", __func__, exp_payload, h.payload);
}

static void pkg_ntoh_hton() {
	NetPkgHdr h(1, 512);
	h.ntoh();
	if (!h.native_ordering)
		fprintf(stderr, "%s: should be native order\n", __func__);
	h.hton();
	if (h.native_ordering)
		fprintf(stderr, "%s: should be network order\n", __func__);
}

static void protocol_version() {
	NetPkg pkg(0, 0);
	uint16_t exp_prot = 1024;
	pkg.set_protocol(exp_prot);
	pkg.hton();
	pkg.ntoh();
	if (exp_prot != pkg.protocol_version())
		fprintf(stderr, "%s: bad protocol version, expected 0x%X, got 0x%X\n", __func__, exp_prot, pkg.protocol_version());
}

void protocol_runall() {
	puts("protocol");
	pkg_init_1_512();
	pkg_ntoh_hton();
	protocol_version();
}

}
