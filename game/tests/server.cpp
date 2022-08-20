#include "../src/server.hpp"

#include <stdexcept>

namespace aoe {

static void server_create_fail() {
	try {
		Server s;
		(void)s;
		fprintf(stderr, "%s: server created with no network subsystem\n", __func__);
	} catch (std::runtime_error&) {}
}

static void server_create_delete() {
	Server s;
	(void)s;
}

static void server_create_twice() {
	Server s, s2;
	(void)s;
	(void)s2;
}

static void server_stop_early() {
	Server s;
	s.stop();
}

static void server_create_port() {
	Server s(1234);
	(void)s;
}

void server_runall() {
	server_create_fail();
	Net n;
	server_create_delete();
	server_create_twice();
	server_stop_early();
	server_create_port();
}

}
